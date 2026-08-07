#include "AssetDatabase.h"

#include <fstream>
#include <iomanip>

namespace Lgt::Assets {
bool AssetDatabase::Load(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in)
        return false;

    _records.clear();
    AssetRecord record;
    std::string sourcePath;
    std::string cookedPath;
    uint32_t type = 0;
    while (in >> std::hex >> record.id.high >> record.id.low >> std::dec >> type >> std::quoted(sourcePath) >> std::quoted(cookedPath)) {
        record.type = static_cast<AssetType>(type);
        record.sourcePath = sourcePath;
        record.cookedPath = cookedPath;
        Upsert(record);
    }
    return true;
}

bool AssetDatabase::Save(const std::filesystem::path& path) const {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);

    std::ofstream out(path, std::ios::trunc);
    if (!out)
        return false;

    for (const auto& [id, record] : _records) {
        out << std::hex << record.id.high << ' ' << record.id.low << std::dec << ' '
            << static_cast<uint32_t>(record.type) << ' '
            << std::quoted(record.sourcePath.generic_string()) << ' '
            << std::quoted(record.cookedPath.generic_string()) << '\n';
    }
    return static_cast<bool>(out);
}

void AssetDatabase::Upsert(AssetRecord record) {
    _records[record.id] = std::move(record);
}

const AssetRecord* AssetDatabase::Find(AssetGuid id) const {
    const auto it = _records.find(id);
    return it == _records.end() ? nullptr : &it->second;
}

} // namespace Lgt::Assets
