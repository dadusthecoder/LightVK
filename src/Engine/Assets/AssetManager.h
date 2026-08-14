#pragma once

#include <filesystem>
#include <memory>
#include <unordered_map>

#include "Assets.h"
#include "AssetDatabase.h"
#include "CookedAsset.h"
#include "Engine/Gpu/Renderer.h"

namespace Lgt::Assets {

struct AssetGuidHash {
    size_t operator()(const AssetGuid& id) const noexcept {
        return static_cast<size_t>(id.high ^ (id.low + 0x9E3779B97F4A7C15ull + (id.high << 6u) + (id.high >> 2u)));
    }
};

struct GpuModel {
    AssetGuid assetId{};
    Gpu::DrawList drawList;
    Gpu::BufferHandle materialBuffer{};
    uint32_t materialBufferIndex = 0;

    std::vector<Gpu::BufferHandle> buffers;
    std::vector<Gpu::TextureHandle> textures;
    std::vector<Gpu::SamplerHandle> samplers;
};

class AssetManager {
public:
    void Init(const std::filesystem::path& cookedRoot = {});
    void Shutdown();

    AssetGuid LoadModel(const std::filesystem::path& path);
    AssetGuid LoadModel(AssetGuid id);

    const ModelAsset* GetModel(AssetGuid id) const;
    const GpuModel* GetGpuModel(AssetGuid id) const;

private:
    std::unordered_map<AssetGuid, std::unique_ptr<ModelAsset>, AssetGuidHash> _models;
    std::unordered_map<AssetGuid, std::unique_ptr<GpuModel>, AssetGuidHash> _gpuModels;
    std::filesystem::path _cookedRoot;
    std::filesystem::path _databasePath;
    AssetDatabase _database;

    std::unique_ptr<GpuModel> UploadModel(const ModelAsset& model);
    AssetGuid InstallModel(ModelAsset model);
};

} // namespace Lgt::Assets
