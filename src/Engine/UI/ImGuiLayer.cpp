#include "ImGuiLayer.h"

#include <volk.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>

#include "Engine/Renderer/Vulkan/Context.h"
#include "Engine/Core/Logger.h"

namespace Lgt {

void ImGuiLayer::Init(GLFWwindow* window, VkFormat colorFormat) {
    VkDescriptorPoolSize       pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                               {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                               {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                               {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                               {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                               {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                               {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                               {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                               {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                               {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                               {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
    VkDescriptorPoolCreateInfo pool_info    = {};
    pool_info.sType                         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets                       = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount                 = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes                    = pool_sizes;

    if (vkCreateDescriptorPool(Vulkan::g_Device->Logical(), &pool_info, nullptr, &descriptorPool_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool for ImGui!");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable docking

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion                = VK_API_VERSION_1_3;
    init_info.Instance                  = Vulkan::g_Instance->Handle();
    init_info.PhysicalDevice            = Vulkan::g_Device->Physical();
    init_info.Device                    = Vulkan::g_Device->Logical();
    init_info.QueueFamily               = Vulkan::g_Device->GraphicsFamily();
    init_info.Queue                     = Vulkan::g_Device->GraphicsQueue();
    init_info.PipelineCache             = VK_NULL_HANDLE;
    init_info.DescriptorPool            = descriptorPool_;
    init_info.MinImageCount             = 3;
    init_info.ImageCount                = 3;
    init_info.Allocator                 = nullptr;
    init_info.CheckVkResultFn           = nullptr;
    init_info.UseDynamicRendering       = true;

    // FIX: Explicitly hand over Volk's function loader to ImGui.
    // This tells line 1345 to use Volk to resolve dynamic rendering functions
    // instead of attempting to fetch them through a missing system SDK path.
    init_info.CheckVkResultFn = nullptr;

#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
    // Ensure volk has bound to instance prior to this point!
#endif

    init_info.PipelineInfoMain.RenderPass  = VK_NULL_HANDLE;
    init_info.PipelineInfoMain.Subpass     = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = {};
    pipelineRenderingCreateInfo.sType                            = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipelineRenderingCreateInfo.colorAttachmentCount             = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats          = &colorFormat;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo       = pipelineRenderingCreateInfo;
    LIGHTVK_INFO("ImGui version: {}", IMGUI_VERSION);
    LIGHTVK_INFO("Device = {}", (void*)init_info.Device);
    LIGHTVK_INFO("Instance = {}", (void*)init_info.Instance);
    LIGHTVK_INFO("Physical = {}", (void*)init_info.PhysicalDevice);
    LIGHTVK_INFO("Queue = {}", (void*)init_info.Queue);
    LIGHTVK_INFO("vkGetDeviceProcAddr = {}", (void*)vkGetDeviceProcAddr);

    auto p = vkGetDeviceProcAddr(Vulkan::g_Device->Logical(), "vkCmdBeginRenderingKHR");

    LIGHTVK_INFO("vkCmdBeginRenderingKHR = {}", (void*)p);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    bool ok = ImGui_ImplVulkan_LoadFunctions(
        VK_API_VERSION_1_4,
        [](const char* name, void*) -> PFN_vkVoidFunction {
            auto p = vkGetInstanceProcAddr(Vulkan::g_Instance->Handle(), name);
            std::cout << "Found it" << std::endl;
            return p;
        },
        nullptr);

    LGT_ASSERT(ok, "Failed to load ImGui Vulkan functions");
            std::cout << "Found it2" << std::endl;
    
    ImGui_ImplVulkan_Init(&init_info);
}

void ImGuiLayer::BeginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::EndFrame(VkCommandBuffer cmd) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void ImGuiLayer::Shutdown() {
    if (Vulkan::g_Device && Vulkan::g_Device->Logical()) {
        vkDeviceWaitIdle(Vulkan::g_Device->Logical());
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (descriptorPool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(Vulkan::g_Device->Logical(), descriptorPool_, nullptr);
            descriptorPool_ = VK_NULL_HANDLE;
        }
    }
}

} // namespace Lgt
