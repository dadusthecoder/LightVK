#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Scene/Components.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Core/Logger.h"

#include <fstream>
#include <vector>
#include <system_error>

namespace Lgt {

SceneSerializer::SceneSerializer(World* world)
    : m_World(world) {}

std::filesystem::path SceneSerializer::ResolveScenePath(const std::filesystem::path& filepath) {
    if (filepath.is_absolute())
        return filepath;

    std::error_code error;
    auto current = std::filesystem::current_path(error);
    if (error)
        return filepath;

    std::filesystem::path projectRoot;
    for (auto directory = current;; directory = directory.parent_path()) {
        const auto directPath = directory / filepath;
        if (std::filesystem::is_regular_file(directPath, error))
            return directPath;

        const auto assetsPath = directory / "Assets" / filepath;
        if (std::filesystem::is_regular_file(assetsPath, error))
            return assetsPath;

        if (std::filesystem::is_regular_file(directory / "CMakeLists.txt", error) &&
            std::filesystem::is_directory(directory / "src", error) &&
            std::filesystem::is_directory(directory / "Assets", error)) {
            projectRoot = directory;
        }

        const auto parent = directory.parent_path();
        if (parent == directory)
            break;
    }

    if (!projectRoot.empty())
        return projectRoot / "Assets" / filepath;
    return filepath;
}

template <typename T> static void SerializeComponentArray(std::ofstream& out, entt::registry& reg) {
    auto     view  = reg.view<T>();
    uint32_t count = (uint32_t)view.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

    for (auto entity : view) {
        entt::entity e = entity;
        out.write(reinterpret_cast<const char*>(&e), sizeof(entt::entity));
        if constexpr (!std::is_empty_v<T>) {
            T& component = view.template get<T>(entity);
            out.write(reinterpret_cast<const char*>(&component), sizeof(T));
        }
    }
}

template <typename T> static void DeserializeComponentArray(std::ifstream& in, entt::registry& reg) {
    uint32_t count;
    in.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));

    for (uint32_t i = 0; i < count; ++i) {
        entt::entity e;
        in.read(reinterpret_cast<char*>(&e), sizeof(entt::entity));

        if (!reg.valid(e)) {
            e = reg.create(e);
        }

        if constexpr (!std::is_empty_v<T>) {
            T component;
            in.read(reinterpret_cast<char*>(&component), sizeof(T));
            reg.emplace_or_replace<T>(e, component);
        } else {
            reg.emplace_or_replace<T>(e);
        }
    }
}

static void SerializeHierarchy(std::ofstream& out, entt::registry& reg) {
    auto view = reg.view<Component::Hierarchy>();
    const uint32_t count = static_cast<uint32_t>(view.size());
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (auto entity : view) {
        const auto& hierarchy = view.get<Component::Hierarchy>(entity);
        const entt::entity parent = hierarchy.parent.Handle();
        out.write(reinterpret_cast<const char*>(&entity), sizeof(entity));
        out.write(reinterpret_cast<const char*>(&parent), sizeof(parent));
    }
}

static bool DeserializeHierarchy(std::ifstream& in, entt::registry& reg, World* world) {
    uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!in)
        return false;

    std::vector<std::pair<entt::entity, entt::entity>> parents;
    parents.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        entt::entity entity = entt::null;
        entt::entity parent = entt::null;
        in.read(reinterpret_cast<char*>(&entity), sizeof(entity));
        in.read(reinterpret_cast<char*>(&parent), sizeof(parent));
        if (!in)
            return false;

        if (!reg.valid(entity))
            (void)reg.create(entity);
        reg.emplace_or_replace<Component::Hierarchy>(entity);
        parents.emplace_back(entity, parent);
    }

    for (const auto& [entity, parent] : parents) {
        if (parent == entt::null || !reg.valid(parent))
            continue;
        world->Graph().SetParent(Entity(parent, world), Entity(entity, world));
    }
    return true;
}

bool SceneSerializer::SerializeBinary(const std::filesystem::path& filepath) {
    const auto resolvedPath = ResolveScenePath(filepath);
    std::error_code error;
    std::filesystem::create_directories(resolvedPath.parent_path(), error);

    std::ofstream out(resolvedPath, std::ios::binary);
    if (!out.is_open()) {
        LIGHTVK_ERROR("Failed to open file for writing: {}", resolvedPath.string());
        return false;
    }

    entt::registry& reg = m_World->Registry();

    // Serialize Tags (special case for std::string)
    auto     tagView  = reg.view<Component::Tag>();
    uint32_t tagCount = (uint32_t)tagView.size();
    out.write(reinterpret_cast<const char*>(&tagCount), sizeof(uint32_t));
    for (auto entity : tagView) {
        entt::entity e = entity;
        out.write(reinterpret_cast<const char*>(&e), sizeof(entt::entity));
        auto&    tag    = tagView.get<Component::Tag>(entity);
        uint32_t strLen = (uint32_t)tag.name.size();
        out.write(reinterpret_cast<const char*>(&strLen), sizeof(uint32_t));
        out.write(tag.name.data(), strLen);
    }

    // Serialize POD Components
    SerializeComponentArray<Component::WorldTransform>(out, reg);
    SerializeComponentArray<Component::LocalTransform>(out, reg);
    SerializeHierarchy(out, reg);
    SerializeComponentArray<Component::Material>(out, reg);
    SerializeComponentArray<Component::DirectionalLight>(out, reg);
    SerializeComponentArray<Component::PointLight>(out, reg);
    SerializeComponentArray<Component::ModelInstance>(out, reg);
    SerializeComponentArray<Component::Camera>(out, reg);

    out.close();
    return true;
}

bool SceneSerializer::DeserializeBinary(const std::filesystem::path& filepath) {
    const auto resolvedPath = ResolveScenePath(filepath);
    std::ifstream in(resolvedPath, std::ios::binary);
    if (!in.is_open()) {
        LIGHTVK_ERROR("Failed to open file for reading: {}", resolvedPath.string());
        return false;
    }

    entt::registry& reg = m_World->Registry();
    reg.clear();

    // Deserialize Tags
    uint32_t tagCount;
    in.read(reinterpret_cast<char*>(&tagCount), sizeof(uint32_t));
    for (uint32_t i = 0; i < tagCount; ++i) {
        entt::entity e;
        in.read(reinterpret_cast<char*>(&e), sizeof(entt::entity));

        uint32_t strLen;
        in.read(reinterpret_cast<char*>(&strLen), sizeof(uint32_t));
        std::string name(strLen, '\0');
        in.read(name.data(), strLen);

        if (!reg.valid(e))
            e = reg.create(e);
        reg.emplace_or_replace<Component::Tag>(e, name);
    }

    // Deserialize POD Components
    DeserializeComponentArray<Component::WorldTransform>(in, reg);
    DeserializeComponentArray<Component::LocalTransform>(in, reg);
    if (!DeserializeHierarchy(in, reg, m_World))
        return false;
    DeserializeComponentArray<Component::Material>(in, reg);
    DeserializeComponentArray<Component::DirectionalLight>(in, reg);
    DeserializeComponentArray<Component::PointLight>(in, reg);
    DeserializeComponentArray<Component::ModelInstance>(in, reg);
    DeserializeComponentArray<Component::Camera>(in, reg);

    in.close();
    return true;
}

} // namespace Lgt
