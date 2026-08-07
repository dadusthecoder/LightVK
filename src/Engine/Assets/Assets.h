#pragma once

#include <filesystem>

#include "AssetTypes.h"

namespace Lgt::Assets {

struct ImportOptions {
    bool generateMissingNormals = true;
    bool generateMissingTangents = true;
    bool generateMipmaps = true;
    bool preserveNodeHierarchy = true;
};

struct ImportResult {
    bool success = false;
    std::string error;
    ModelAsset model;
};

AssetGuid AssetIdForPath(const std::filesystem::path& path);

ImportResult ImportGltf(const std::filesystem::path& path,
                        const ImportOptions& options = {});

} // namespace Lgt::Assets
