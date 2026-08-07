#pragma once

#include <filesystem>

#include "AssetTypes.h"

namespace Lgt::Assets {

constexpr uint32_t CookedModelMagic = 0x4C4D4F44; // LMOD
constexpr uint16_t CookedModelVersion = 1;

struct CookedModelHeader {
    uint32_t magic = CookedModelMagic;
    uint16_t version = CookedModelVersion;
    uint16_t reserved = 0;
    AssetGuid assetId{};
    uint64_t sourceSize = 0;
    int64_t sourceTimestamp = 0;
};

bool SaveCookedModel(const std::filesystem::path& cookedPath,
                     const std::filesystem::path& sourcePath,
                     const ModelAsset& model);

bool LoadCookedModel(const std::filesystem::path& cookedPath,
                     ModelAsset* model);

bool IsCookedModelCurrent(const std::filesystem::path& cookedPath,
                          const std::filesystem::path& sourcePath);

} // namespace Lgt::Assets
