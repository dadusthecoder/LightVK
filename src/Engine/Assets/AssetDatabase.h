#pragma once

#include <filesystem>
#include <unordered_map>

#include "AssetTypes.h"

namespace Lgt::Assets {

struct AssetRecord {
    AssetGuid id{};
    AssetType type = AssetType::Unknown;
    std::filesystem::path sourcePath;
    std::filesystem::path cookedPath;
};

struct AssetDatabaseGuidHash {
    size_t operator()(const AssetGuid& id) const noexcept {
        return static_cast<size_t>(id.high ^ (id.low + 0x9E3779B97F4A7C15ull + (id.high << 6u) + (id.high >> 2u)));
    }
};

class AssetDatabase {
public:
    bool Load(const std::filesystem::path& path);
    bool Save(const std::filesystem::path& path) const;

    void Upsert(AssetRecord record);
    const AssetRecord* Find(AssetGuid id) const;

private:
    std::unordered_map<AssetGuid, AssetRecord, AssetDatabaseGuidHash> _records;
};

} // namespace Lgt::Assets
