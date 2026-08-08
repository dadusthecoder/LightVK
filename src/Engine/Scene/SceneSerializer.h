#pragma once
#include "Engine/Scene/World.h"
#include <string>
#include <filesystem>

namespace Lgt {

class SceneSerializer {
public:
    SceneSerializer(World* world);

    // Save/Load in binary format
    bool SerializeBinary(const std::filesystem::path& filepath);
    bool DeserializeBinary(const std::filesystem::path& filepath);

    // Resolves relative scene names from the launch directory or the project Assets directory.
    static std::filesystem::path ResolveScenePath(const std::filesystem::path& filepath);

private:
    World* m_World;
};

} // namespace Lgt
