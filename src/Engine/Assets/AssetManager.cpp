#include "AssetManager.h"

#include "Engine/Core/Logger.h"
#include "Engine/Renderer/Gpu/Context.h"
#include "Engine/Renderer/Vulkan/Context.h"

#include <algorithm>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>

namespace Lgt::Assets {
namespace {

Gpu::SamplerDesc ToGpuSampler(const SamplerAsset& source) {
    Gpu::SamplerDesc result;

    result.magFilter = source.magFilter == SamplerFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    result.minFilter = source.minFilter == SamplerFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    result.mipmapMode = source.mipmapMode == SamplerMipmapMode::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST
                                                                        : VK_SAMPLER_MIPMAP_MODE_LINEAR;

    auto toAddressMode = [](SamplerAddressMode mode) {
        switch (mode) {
        case SamplerAddressMode::ClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case SamplerAddressMode::MirroredRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        default:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    };

    result.addressModeU = toAddressMode(source.addressModeU);
    result.addressModeV = toAddressMode(source.addressModeV);
    result.addressModeW = toAddressMode(source.addressModeW);
    result.debugName = source.name;
    return result;
}

VkFormat ToGpuFormat(const TextureAsset& texture) {
    if (texture.colorSpace == TextureColorSpace::SRGB)
        return VK_FORMAT_R8G8B8A8_SRGB;
    return VK_FORMAT_R8G8B8A8_UNORM;
}

glm::mat4 ToGlmTransform(const Transform& transform) {
    glm::mat4 result(1.0f);
    result = glm::translate(result, {transform.position.x, transform.position.y, transform.position.z});
    result *= glm::mat4_cast(glm::quat(transform.rotation.w,
                                       transform.rotation.x,
                                       transform.rotation.y,
                                       transform.rotation.z));
    result = glm::scale(result, {transform.scale.x, transform.scale.y, transform.scale.z});
    return result;
}

struct MaterialGpuData {
    glm::vec4 baseColorFactor{1.0f};
    glm::vec4 emissiveFactor{0.0f};
    glm::vec4 scalarFactors{0.0f}; // metallic, roughness, normal scale, occlusion strength

    uint32_t baseColorTexture = 0xFFFFFFFFu;
    uint32_t baseColorSampler = 0xFFFFFFFFu;
    uint32_t normalTexture = 0xFFFFFFFFu;
    uint32_t normalSampler = 0xFFFFFFFFu;
    uint32_t metallicRoughnessTexture = 0xFFFFFFFFu;
    uint32_t metallicRoughnessSampler = 0xFFFFFFFFu;
    uint32_t occlusionTexture = 0xFFFFFFFFu;
    uint32_t occlusionSampler = 0xFFFFFFFFu;
    uint32_t emissiveTexture = 0xFFFFFFFFu;
    uint32_t emissiveSampler = 0xFFFFFFFFu;
};

uint32_t FindTextureDescriptor(const TextureSlot& slot,
                               const std::unordered_map<AssetGuid, uint32_t, AssetGuidHash>& descriptors) {
    const auto it = descriptors.find(slot.texture.id);
    return it == descriptors.end() ? 0xFFFFFFFFu : it->second;
}

uint32_t FindSamplerDescriptor(const TextureSlot& slot,
                               const std::unordered_map<AssetGuid, uint32_t, AssetGuidHash>& descriptors) {
    const auto it = descriptors.find(slot.sampler.id);
    return it == descriptors.end() ? 0xFFFFFFFFu : it->second;
}

} // namespace

void AssetManager::Init(const std::filesystem::path& cookedRoot) {
    _cookedRoot = cookedRoot.empty() ? (std::filesystem::current_path() / "Cooked") : cookedRoot;
    _databasePath = _cookedRoot / "AssetDatabase.txt";
    std::error_code error;
    std::filesystem::create_directories(_cookedRoot, error);
    _database.Load(_databasePath);
    LIGHTVK_INFO("AssetManager Initialized; cooked root '{}'", _cookedRoot.string());
}

void AssetManager::Shutdown() {
    if (Gpu::Resources != nullptr) {
        for (auto& [id, model] : _gpuModels) {
            for (auto handle : model->samplers)
                Gpu::Resources->DestroySampler(handle);
            for (auto handle : model->textures)
                Gpu::Resources->DestroyTexture(handle);
            for (auto handle : model->buffers)
                Gpu::Resources->DestroyBuffer(handle);
            if (model->materialBuffer.IsValid())
                Gpu::Resources->DestroyBuffer(model->materialBuffer);
        }
    }

    _gpuModels.clear();
    _models.clear();
    LIGHTVK_INFO("AssetManager Shutdown");
}

AssetGuid AssetManager::LoadModel(const std::filesystem::path& path) {
    const AssetGuid expectedId = AssetIdForPath(path);
    if (_gpuModels.contains(expectedId))
        return expectedId;

    const auto cookedPath = _cookedRoot /
                            (std::to_string(expectedId.high) + "_" + std::to_string(expectedId.low) + ".lmodel");

    ModelAsset model;
    if (IsCookedModelCurrent(cookedPath, path)) {
        if (!LoadCookedModel(cookedPath, &model))
            LIGHTVK_WARN("AssetManager: cooked asset '{}' is invalid; reimporting", cookedPath.string());
    }

    if (model.id != expectedId) {
        auto imported = ImportGltf(path);
        if (!imported.success) {
            LIGHTVK_ERROR("AssetManager: {}", imported.error);
            return {};
        }
        model = std::move(imported.model);
        if (!SaveCookedModel(cookedPath, path, model))
            LIGHTVK_WARN("AssetManager: failed to write cooked asset '{}'", cookedPath.string());
    }

    const AssetGuid id = model.id;
    _database.Upsert({id, AssetType::Model, path, cookedPath.filename()});
    _database.Save(_databasePath);

    InstallModel(std::move(model));
    LIGHTVK_INFO("AssetManager: model '{}' is resident", path.string());
    return id;
}

AssetGuid AssetManager::LoadModel(AssetGuid id) {
    if (_gpuModels.contains(id))
        return id;

    const auto* record = _database.Find(id);
    if (record == nullptr) {
        LIGHTVK_ERROR("AssetManager: no database record for model {:016x}:{:016x}", id.high, id.low);
        return {};
    }

    auto cookedPath = record->cookedPath;
    if (!cookedPath.is_absolute())
        cookedPath = _cookedRoot / cookedPath;
    if (!std::filesystem::exists(cookedPath))
        cookedPath = _cookedRoot / cookedPath.filename();

    ModelAsset model;
    if (!LoadCookedModel(cookedPath, &model)) {
        LIGHTVK_ERROR("AssetManager: failed to load cooked model '{}'", cookedPath.string());
        return {};
    }

    return InstallModel(std::move(model));
}

const ModelAsset* AssetManager::GetModel(AssetGuid id) const {
    const auto it = _models.find(id);
    return it == _models.end() ? nullptr : it->second.get();
}

const GpuModel* AssetManager::GetGpuModel(AssetGuid id) const {
    const auto it = _gpuModels.find(id);
    return it == _gpuModels.end() ? nullptr : it->second.get();
}

AssetGuid AssetManager::InstallModel(ModelAsset model) {
    const AssetGuid id = model.id;
    auto modelOwner = std::make_unique<ModelAsset>(std::move(model));
    auto gpuModel = UploadModel(*modelOwner);
    if (!gpuModel)
        return {};

    _models[id] = std::move(modelOwner);
    _gpuModels[id] = std::move(gpuModel);
    return id;
}

std::unique_ptr<GpuModel> AssetManager::UploadModel(const ModelAsset& model) {
    auto result = std::make_unique<GpuModel>();
    result->assetId = model.id;

    std::unordered_map<AssetGuid, uint32_t, AssetGuidHash> textureDescriptors;
    std::unordered_map<AssetGuid, uint32_t, AssetGuidHash> samplerDescriptors;

    for (const auto& sampler : model.samplers) {
        const auto handle = Gpu::Resources->CreateSampler(ToGpuSampler(sampler));
        if (!handle.IsValid()) {
            LIGHTVK_ERROR("AssetManager: failed to create sampler '{}'", sampler.name);
            return nullptr;
        }
        const auto* gpuSampler = Gpu::Resources->GetSampler(handle);
        if (gpuSampler == nullptr)
            return nullptr;
        samplerDescriptors[sampler.id] = gpuSampler->descriptorIndex;
        result->samplers.push_back(handle);
    }

    for (const auto& texture : model.textures) {
        if (texture.width == 0 || texture.height == 0 || texture.data.empty()) {
            LIGHTVK_WARN("AssetManager: skipping empty texture '{}'", texture.name);
            continue;
        }

        auto textureHandle = Gpu::Resources->CreateTexture(
            Gpu::TextureDesc::Texture2D({texture.width, texture.height}, ToGpuFormat(texture), texture.name));
        auto* gpuTexture = Gpu::Resources->GetTexture(textureHandle);
        if (!textureHandle.IsValid() || gpuTexture == nullptr) {
            LIGHTVK_ERROR("AssetManager: failed to create texture '{}'", texture.name);
            return nullptr;
        }

        Vulkan::g_Uploader->uploadTexture(gpuTexture->image,
                                          texture.data.data(),
                                          static_cast<uint32_t>(texture.data.size()),
                                          Vulkan::TextureCopy::FullTexture(texture.width, texture.height));

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = gpuTexture->image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = gpuTexture->format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        const uint32_t descriptorIndex = Gpu::ResourceHeap->AllocateTexture(
            textureHandle, viewInfo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        textureDescriptors[texture.id] = descriptorIndex;
        gpuTexture->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        result->textures.push_back(textureHandle);
    }

    std::vector<MaterialGpuData> materialData;
    materialData.reserve(std::max<size_t>(model.materials.size(), 1));
    if (model.materials.empty())
        materialData.emplace_back();
    for (const auto& material : model.materials) {
        MaterialGpuData gpuMaterial;
        gpuMaterial.baseColorFactor = {material.baseColorFactor.x,
                                       material.baseColorFactor.y,
                                       material.baseColorFactor.z,
                                       material.baseColorFactor.w};
        gpuMaterial.emissiveFactor = {material.emissiveFactor.x,
                                      material.emissiveFactor.y,
                                      material.emissiveFactor.z,
                                      material.emissiveStrength};
        gpuMaterial.scalarFactors = {material.metallicFactor,
                                     material.roughnessFactor,
                                     material.normalScale,
                                     material.occlusionStrength};
        gpuMaterial.baseColorTexture = FindTextureDescriptor(material.baseColor, textureDescriptors);
        gpuMaterial.baseColorSampler = FindSamplerDescriptor(material.baseColor, samplerDescriptors);
        gpuMaterial.normalTexture = FindTextureDescriptor(material.normal, textureDescriptors);
        gpuMaterial.normalSampler = FindSamplerDescriptor(material.normal, samplerDescriptors);
        gpuMaterial.metallicRoughnessTexture = FindTextureDescriptor(material.metallicRoughness, textureDescriptors);
        gpuMaterial.metallicRoughnessSampler = FindSamplerDescriptor(material.metallicRoughness, samplerDescriptors);
        gpuMaterial.occlusionTexture = FindTextureDescriptor(material.occlusion, textureDescriptors);
        gpuMaterial.occlusionSampler = FindSamplerDescriptor(material.occlusion, samplerDescriptors);
        gpuMaterial.emissiveTexture = FindTextureDescriptor(material.emissive, textureDescriptors);
        gpuMaterial.emissiveSampler = FindSamplerDescriptor(material.emissive, samplerDescriptors);
        materialData.push_back(gpuMaterial);
    }

    result->materialBuffer = Gpu::Resources->CreateBuffer(
        Gpu::BufferDesc::SSBO(materialData.size() * sizeof(MaterialGpuData), false, model.name + ".Materials"));
    auto* materialBuffer = Gpu::Resources->GetBuffer(result->materialBuffer);
    if (!result->materialBuffer.IsValid() || materialBuffer == nullptr)
        return nullptr;
    Vulkan::g_Uploader->UploadBuffer(materialBuffer->buffer,
                                      materialData.data(),
                                      static_cast<uint32_t>(materialData.size() * sizeof(MaterialGpuData)));
    result->materialBufferIndex = Gpu::ResourceHeap->AllocateSSBO(result->materialBuffer);

    std::vector<glm::mat4> nodeWorldTransforms(model.nodes.size(), glm::mat4(1.0f));
    std::vector<uint8_t> resolvedNodes(model.nodes.size(), 0);
    std::function<glm::mat4(size_t)> resolveNodeTransform = [&](size_t nodeIndex) -> glm::mat4 {
        if (resolvedNodes[nodeIndex] == 2)
            return nodeWorldTransforms[nodeIndex];

        const auto& node = model.nodes[nodeIndex];
        const glm::mat4 local = ToGlmTransform(node.localTransform);
        if (resolvedNodes[nodeIndex] == 1) {
            LIGHTVK_WARN("AssetManager: hierarchy cycle detected at node '{}'; using local transform", node.name);
            return local;
        }
        resolvedNodes[nodeIndex] = 1;
        if (node.parentIndex == InvalidIndex || node.parentIndex >= model.nodes.size()) {
            nodeWorldTransforms[nodeIndex] = local;
        } else if (node.parentIndex == nodeIndex) {
            LIGHTVK_WARN("AssetManager: node '{}' is its own parent; ignoring hierarchy cycle", node.name);
            nodeWorldTransforms[nodeIndex] = local;
        } else {
            nodeWorldTransforms[nodeIndex] = resolveNodeTransform(node.parentIndex) * local;
        }
        resolvedNodes[nodeIndex] = 2;
        return nodeWorldTransforms[nodeIndex];
    };
    for (size_t nodeIndex = 0; nodeIndex < model.nodes.size(); ++nodeIndex)
        resolveNodeTransform(nodeIndex);

    for (size_t meshIndex = 0; meshIndex < model.meshes.size(); ++meshIndex) {
        const auto& mesh = model.meshes[meshIndex];
        if (mesh.vertices.empty() || mesh.indices.empty())
            continue;

        const auto vertexBuffer = Gpu::Resources->CreateBuffer(
            Gpu::BufferDesc::SSBO(mesh.vertices.size() * sizeof(MeshVertex), false, mesh.name + ".Vertices"));
        const auto indexBuffer = Gpu::Resources->CreateBuffer(
            Gpu::BufferDesc::SSBO(mesh.indices.size() * sizeof(uint32_t), false, mesh.name + ".Indices"));

        auto* gpuVertexBuffer = Gpu::Resources->GetBuffer(vertexBuffer);
        auto* gpuIndexBuffer = Gpu::Resources->GetBuffer(indexBuffer);
        if (!vertexBuffer.IsValid() || !indexBuffer.IsValid() || gpuVertexBuffer == nullptr || gpuIndexBuffer == nullptr)
            return nullptr;

        Vulkan::g_Uploader->UploadBuffer(gpuVertexBuffer->buffer,
                                          mesh.vertices.data(),
                                          static_cast<uint32_t>(mesh.vertices.size() * sizeof(MeshVertex)));
        Vulkan::g_Uploader->UploadBuffer(gpuIndexBuffer->buffer,
                                          mesh.indices.data(),
                                          static_cast<uint32_t>(mesh.indices.size() * sizeof(uint32_t)));

        const uint32_t vertexDescriptor = Gpu::ResourceHeap->AllocateSSBO(vertexBuffer);
        const uint32_t indexDescriptor = Gpu::ResourceHeap->AllocateSSBO(indexBuffer);
        result->buffers.push_back(vertexBuffer);
        result->buffers.push_back(indexBuffer);

        for (size_t nodeIndex = 0; nodeIndex < model.nodes.size(); ++nodeIndex) {
            const auto& node = model.nodes[nodeIndex];
            if (node.meshIndex != meshIndex)
                continue;

            glm::mat4 transform = nodeWorldTransforms[nodeIndex];
            for (const auto& primitive : mesh.primitives) {
                Gpu::DrawCommand command{};
                command.vertexBufferIndex = vertexDescriptor;
                command.indexBufferIndex = indexDescriptor;
                command.indexOffset = primitive.indexOffset;
                command.materialIndex = primitive.materialIndex == InvalidIndex ? 0 : primitive.materialIndex;
                command.materialBufferIndex = result->materialBufferIndex;
                command.transform = transform;
                result->drawList.commands.push_back(command);
                result->drawList.indexCounts.push_back(primitive.indexCount);
            }
        }

        if (model.nodes.empty()) {
            for (const auto& primitive : mesh.primitives) {
                Gpu::DrawCommand command{};
                command.vertexBufferIndex = vertexDescriptor;
                command.indexBufferIndex = indexDescriptor;
                command.indexOffset = primitive.indexOffset;
                command.materialIndex = primitive.materialIndex == InvalidIndex ? 0 : primitive.materialIndex;
                command.materialBufferIndex = result->materialBufferIndex;
                result->drawList.commands.push_back(command);
                result->drawList.indexCounts.push_back(primitive.indexCount);
            }
        }
    }

    Vulkan::g_Uploader->Flush();
    return result;
}

} // namespace Lgt::Assets
