#include "EditorTestRunner.h"

#if defined(LIGHTVK_EDITOR_TESTS)

#include <imgui.h>

#include <exception>
#include <memory>
#include <utility>

#include "Engine/Assets/Assets.h"
#include "Engine/Core/Logger.h"
#include "Engine/Gpu/Context.h"
#include "Engine/Gpu/Passes/Gbuffer.h"
#include "Engine/Gpu/ResourceManager.h"
#include "Engine/Gpu/Vulkan/Context.h"
#include "Engine/Gpu/Vulkan/Uploader.h"

namespace Lgt::Editor::Tests {
namespace {

struct TextureTestState {
    Gpu::TextureHandle texture;
    Gpu::SamplerHandle sampler;
    uint32_t           textureDescriptorIndex = 0;
};

void DestroyTextureTestResources(const std::shared_ptr<TextureTestState>& state) {
    if (state->sampler.IsValid()) {
        Gpu::Resources->DestroySampler(state->sampler);
        state->sampler = {};
    }

    if (state->texture.IsValid()) {
        Gpu::Resources->DestroyTexture(state->texture);
        state->texture = {};
    }

    state->textureDescriptorIndex = 0;
}

} // namespace

void EditorTestRunner::Init(World* world) {
    _world = world;
    RegisterBuiltInTests();
}

void EditorTestRunner::RegisterTest(std::string name, std::string description, TestFunction run, CleanupFunction cleanup) {
    _tests.push_back({std::move(name), std::move(description), std::move(run), std::move(cleanup), {}, 0});
}

void EditorTestRunner::RegisterBuiltInTests() {
    RegisterTest("Logging", "Emits the standard logger levels so the editor console and file sink can be checked.", [] {
        LIGHTVK_TRACE("Editor test trace");
        LIGHTVK_INFO("Editor test info");
        LIGHTVK_WARN("Editor test warning");
        LIGHTVK_ERROR("Editor test error");
        return TestResult::Pass("All logger levels emitted");
    });

    auto textureState = std::make_shared<TextureTestState>();
    RegisterTest(
        "Bright Red Texture",
        "Creates a 1x1 RGBA8 image, uploads a red texel, builds its image view descriptor, and creates a sampler.",
        [textureState] {
            Assets::TextureAsset textureData;
            textureData.width  = 1;
            textureData.height = 1;
            textureData.data   = {255, 0, 0, 255};

            textureState->texture = Gpu::Resources->CreateTexture(Gpu::TextureDesc::Texture2D(
                {textureData.width, textureData.height}, VK_FORMAT_R8G8B8A8_UNORM, "BrightRedTexture"));
            auto* gpuTexture      = Gpu::Resources->GetTexture(textureState->texture);
            if (gpuTexture == nullptr)
                return TestResult::Fail("ResourceManager returned an invalid texture");

            Vulkan::g_Uploader->uploadTexture(gpuTexture->image,
                                              textureData.data.data(),
                                              static_cast<uint32_t>(textureData.data.size()),
                                              Vulkan::TextureCopy::FullTexture(textureData.width, textureData.height));

            VkImageViewCreateInfo imageViewInfo{};
            imageViewInfo.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.image      = gpuTexture->image;
            imageViewInfo.viewType   = VK_IMAGE_VIEW_TYPE_2D;
            imageViewInfo.format     = gpuTexture->format;
            imageViewInfo.components = {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
            };
            imageViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewInfo.subresourceRange.baseMipLevel   = 0;
            imageViewInfo.subresourceRange.levelCount     = gpuTexture->mipLevels;
            imageViewInfo.subresourceRange.baseArrayLayer = 0;
            imageViewInfo.subresourceRange.layerCount     = gpuTexture->arrayLayers;

            textureState->textureDescriptorIndex = Gpu::ResourceHeap->AllocateTexture(
                textureState->texture, imageViewInfo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            Gpu::SamplerDesc samplerDesc;
            samplerDesc.debugName = "EditorTestSampler";
            textureState->sampler = Gpu::Resources->CreateSampler(samplerDesc);
            if (!textureState->sampler.IsValid())
                return TestResult::Fail("ResourceManager failed to create the sampler");

            Vulkan::g_Uploader->Flush();
            LIGHTVK_INFO("Editor test created bright red texture at descriptor index {}", textureState->textureDescriptorIndex);
            return TestResult::Pass("Texture uploaded and descriptors allocated");
        },
        [textureState] { DestroyTextureTestResources(textureState); });

    RegisterTest(
        "GBuffer Render Graph",
        "Builds and executes the current GBuffer render-graph setup.",
        [] {
            Gpu::FrameGraph->AddPass<Gpu::Pass::GBuffer>();
            Gpu::FrameGraph->Compile();
            Gpu::FrameGraph->Execute();
            return TestResult::Pass("GBuffer graph compiled and executed");
        },
        [] {
            Gpu::Pass::GBuffer::Reset();
            Gpu::FrameGraph->Reset();
        });
}

void EditorTestRunner::RunTest(TestEntry& test) {
    ++test.runCount;
    try {
        if (test.cleanup)
            test.cleanup();
        test.lastResult = test.run();
    } catch (const std::exception& exception) {
        test.lastResult = TestResult::Fail(exception.what());
    } catch (...) {
        test.lastResult = TestResult::Fail("Unknown exception");
    }

    if (test.lastResult.passed)
        LIGHTVK_INFO("Editor test '{}' passed: {}", test.name, test.lastResult.message);
    else
        LIGHTVK_ERROR("Editor test '{}' failed: {}", test.name, test.lastResult.message);
}

void EditorTestRunner::DrawUi() {
    if (!ImGui::Begin("Editor Tests")) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Run All")) {
        for (auto& test : _tests)
            RunTest(test);
    }
    ImGui::SameLine();
    ImGui::Text("%zu tests", _tests.size());

    for (size_t index = 0; index < _tests.size(); ++index) {
        auto& test = _tests[index];
        ImGui::PushID(static_cast<int>(index));
        ImGui::Separator();
        ImGui::TextUnformatted(test.name.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Run"))
            RunTest(test);

        ImGui::TextWrapped("%s", test.description.c_str());
        if (test.runCount > 0) {
            ImVec4 color = test.lastResult.passed ? ImVec4(0.35f, 1.0f, 0.35f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::Text("%s: %s", test.lastResult.passed ? "PASS" : "FAIL", test.lastResult.message.c_str());
            ImGui::PopStyleColor();
            ImGui::Text("Runs: %u", test.runCount);
        }
        ImGui::PopID();
    }

    ImGui::End();
}

void EditorTestRunner::Shutdown() {
    for (auto& test : _tests) {
        if (test.cleanup)
            test.cleanup();
    }
    _tests.clear();
    _world = nullptr;
}

} // namespace Lgt::Editor::Tests

#endif // LIGHTVK_EDITOR_TESTS
