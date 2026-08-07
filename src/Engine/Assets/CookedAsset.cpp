#include "CookedAsset.h"

#include "Engine/Core/Logger.h"

#include <fstream>
#include <limits>
#include <type_traits>

namespace Lgt::Assets {
namespace {

constexpr uint32_t MaxSerializedCount = 50'000'000;

class Writer {
public:
    explicit Writer(std::ofstream& stream)
        : _stream(stream) {}

    template <typename T>
    void Value(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>);
        _stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    void String(const std::string& value) {
        const uint32_t size = static_cast<uint32_t>(value.size());
        Value(size);
        _stream.write(value.data(), size);
    }

    template <typename T>
    void Vector(const std::vector<T>& values) {
        const uint32_t count = static_cast<uint32_t>(values.size());
        Value(count);
        if (count > 0)
            _stream.write(reinterpret_cast<const char*>(values.data()), sizeof(T) * count);
    }

    bool Good() const { return static_cast<bool>(_stream); }

private:
    std::ofstream& _stream;
};

class Reader {
public:
    explicit Reader(std::ifstream& stream)
        : _stream(stream) {}

    template <typename T>
    bool Value(T& value) {
        static_assert(std::is_trivially_copyable_v<T>);
        _stream.read(reinterpret_cast<char*>(&value), sizeof(T));
        return static_cast<bool>(_stream);
    }

    bool String(std::string& value) {
        uint32_t size = 0;
        if (!Value(size) || size > MaxSerializedCount)
            return false;
        value.resize(size);
        if (size > 0)
            _stream.read(value.data(), size);
        return static_cast<bool>(_stream);
    }

    template <typename T>
    bool Vector(std::vector<T>& values) {
        uint32_t count = 0;
        if (!Value(count) || count > MaxSerializedCount)
            return false;
        values.resize(count);
        if (count > 0)
            _stream.read(reinterpret_cast<char*>(values.data()), sizeof(T) * count);
        return static_cast<bool>(_stream);
    }

private:
    std::ifstream& _stream;
};

void WriteSlot(Writer& writer, const TextureSlot& slot) {
    writer.Value(slot.texture.id);
    writer.Value(slot.texture.type);
    writer.Value(slot.sampler.id);
    writer.Value(slot.sampler.type);
    writer.Value(slot.uvSet);
    writer.Value(slot.strength);
}

bool ReadSlot(Reader& reader, TextureSlot& slot) {
    return reader.Value(slot.texture.id) && reader.Value(slot.texture.type) &&
           reader.Value(slot.sampler.id) && reader.Value(slot.sampler.type) &&
           reader.Value(slot.uvSet) && reader.Value(slot.strength);
}

void WriteModel(Writer& writer, const ModelAsset& model) {
    writer.Value(model.id);
    writer.String(model.name);

    const uint32_t nodeCount = static_cast<uint32_t>(model.nodes.size());
    writer.Value(nodeCount);
    for (const auto& node : model.nodes) {
        writer.String(node.name);
        writer.Value(node.parentIndex);
        writer.Value(node.meshIndex);
        writer.Value(node.localTransform);
        writer.Value(node.metadata.mobility);
        writer.Value(node.metadata.castShadow);
        writer.Value(node.metadata.receiveShadow);
        writer.String(node.metadata.role);
    }

    const uint32_t meshCount = static_cast<uint32_t>(model.meshes.size());
    writer.Value(meshCount);
    for (const auto& mesh : model.meshes) {
        writer.Value(mesh.id);
        writer.String(mesh.name);
        writer.Vector(mesh.vertices);
        writer.Vector(mesh.indices);
        writer.Vector(mesh.primitives);
        writer.Value(mesh.bounds);
    }

    const uint32_t materialCount = static_cast<uint32_t>(model.materials.size());
    writer.Value(materialCount);
    for (const auto& material : model.materials) {
        writer.Value(material.id);
        writer.String(material.name);
        writer.Value(material.baseColorFactor);
        writer.Value(material.emissiveFactor);
        writer.Value(material.metallicFactor);
        writer.Value(material.roughnessFactor);
        writer.Value(material.normalScale);
        writer.Value(material.occlusionStrength);
        writer.Value(material.emissiveStrength);
        writer.Value(material.alphaCutoff);
        WriteSlot(writer, material.baseColor);
        WriteSlot(writer, material.normal);
        WriteSlot(writer, material.metallicRoughness);
        WriteSlot(writer, material.occlusion);
        WriteSlot(writer, material.emissive);
        writer.Value(material.alphaMode);
        writer.Value(material.doubleSided);
    }

    const uint32_t textureCount = static_cast<uint32_t>(model.textures.size());
    writer.Value(textureCount);
    for (const auto& texture : model.textures) {
        writer.Value(texture.id);
        writer.String(texture.name);
        writer.Value(texture.semantic);
        writer.Value(texture.colorSpace);
        writer.Value(texture.format);
        writer.Value(texture.width);
        writer.Value(texture.height);
        writer.Value(texture.mipLevels);
        writer.Vector(texture.data);
    }

    const uint32_t samplerCount = static_cast<uint32_t>(model.samplers.size());
    writer.Value(samplerCount);
    for (const auto& sampler : model.samplers) {
        writer.Value(sampler.id);
        writer.String(sampler.name);
        writer.Value(sampler.magFilter);
        writer.Value(sampler.minFilter);
        writer.Value(sampler.mipmapMode);
        writer.Value(sampler.addressModeU);
        writer.Value(sampler.addressModeV);
        writer.Value(sampler.addressModeW);
    }
}

bool ReadModel(Reader& reader, ModelAsset& model) {
    if (!reader.Value(model.id) || !reader.String(model.name))
        return false;

    uint32_t nodeCount = 0;
    if (!reader.Value(nodeCount) || nodeCount > MaxSerializedCount)
        return false;
    model.nodes.resize(nodeCount);
    for (auto& node : model.nodes) {
        if (!reader.String(node.name) || !reader.Value(node.parentIndex) || !reader.Value(node.meshIndex) ||
            !reader.Value(node.localTransform) || !reader.Value(node.metadata.mobility) ||
            !reader.Value(node.metadata.castShadow) || !reader.Value(node.metadata.receiveShadow) ||
            !reader.String(node.metadata.role))
            return false;
    }

    uint32_t meshCount = 0;
    if (!reader.Value(meshCount) || meshCount > MaxSerializedCount)
        return false;
    model.meshes.resize(meshCount);
    for (auto& mesh : model.meshes) {
        if (!reader.Value(mesh.id) || !reader.String(mesh.name) || !reader.Vector(mesh.vertices) ||
            !reader.Vector(mesh.indices) || !reader.Vector(mesh.primitives) || !reader.Value(mesh.bounds))
            return false;
    }

    uint32_t materialCount = 0;
    if (!reader.Value(materialCount) || materialCount > MaxSerializedCount)
        return false;
    model.materials.resize(materialCount);
    for (auto& material : model.materials) {
        if (!reader.Value(material.id) || !reader.String(material.name) || !reader.Value(material.baseColorFactor) ||
            !reader.Value(material.emissiveFactor) || !reader.Value(material.metallicFactor) ||
            !reader.Value(material.roughnessFactor) || !reader.Value(material.normalScale) ||
            !reader.Value(material.occlusionStrength) || !reader.Value(material.emissiveStrength) ||
            !reader.Value(material.alphaCutoff) || !ReadSlot(reader, material.baseColor) ||
            !ReadSlot(reader, material.normal) || !ReadSlot(reader, material.metallicRoughness) ||
            !ReadSlot(reader, material.occlusion) || !ReadSlot(reader, material.emissive) ||
            !reader.Value(material.alphaMode) || !reader.Value(material.doubleSided))
            return false;
    }

    uint32_t textureCount = 0;
    if (!reader.Value(textureCount) || textureCount > MaxSerializedCount)
        return false;
    model.textures.resize(textureCount);
    for (auto& texture : model.textures) {
        if (!reader.Value(texture.id) || !reader.String(texture.name) || !reader.Value(texture.semantic) ||
            !reader.Value(texture.colorSpace) || !reader.Value(texture.format) || !reader.Value(texture.width) ||
            !reader.Value(texture.height) || !reader.Value(texture.mipLevels) || !reader.Vector(texture.data))
            return false;
    }

    uint32_t samplerCount = 0;
    if (!reader.Value(samplerCount) || samplerCount > MaxSerializedCount)
        return false;
    model.samplers.resize(samplerCount);
    for (auto& sampler : model.samplers) {
        if (!reader.Value(sampler.id) || !reader.String(sampler.name) || !reader.Value(sampler.magFilter) ||
            !reader.Value(sampler.minFilter) || !reader.Value(sampler.mipmapMode) ||
            !reader.Value(sampler.addressModeU) || !reader.Value(sampler.addressModeV) ||
            !reader.Value(sampler.addressModeW))
            return false;
    }

    return true;
}

bool SourceInfo(const std::filesystem::path& path, uint64_t& size, int64_t& timestamp) {
    std::error_code error;
    size = std::filesystem::file_size(path, error);
    if (error)
        return false;
    timestamp = std::filesystem::last_write_time(path, error).time_since_epoch().count();
    return !error;
}

} // namespace

bool SaveCookedModel(const std::filesystem::path& cookedPath,
                     const std::filesystem::path& sourcePath,
                     const ModelAsset& model) {
    uint64_t sourceSize = 0;
    int64_t sourceTimestamp = 0;
    if (!SourceInfo(sourcePath, sourceSize, sourceTimestamp))
        return false;

    std::error_code error;
    std::filesystem::create_directories(cookedPath.parent_path(), error);

    std::ofstream out(cookedPath, std::ios::binary | std::ios::trunc);
    if (!out)
        return false;

    CookedModelHeader header;
    header.assetId = model.id;
    header.sourceSize = sourceSize;
    header.sourceTimestamp = sourceTimestamp;

    Writer writer(out);
    writer.Value(header);
    WriteModel(writer, model);
    return writer.Good();
}

bool LoadCookedModel(const std::filesystem::path& cookedPath, ModelAsset* model) {
    if (model == nullptr)
        return false;

    std::ifstream in(cookedPath, std::ios::binary);
    if (!in)
        return false;

    CookedModelHeader header;
    Reader reader(in);
    if (!reader.Value(header) || header.magic != CookedModelMagic || header.version != CookedModelVersion)
        return false;

    ModelAsset loaded;
    if (!ReadModel(reader, loaded) || loaded.id != header.assetId)
        return false;

    *model = std::move(loaded);
    return true;
}

bool IsCookedModelCurrent(const std::filesystem::path& cookedPath,
                          const std::filesystem::path& sourcePath) {
    uint64_t sourceSize = 0;
    int64_t sourceTimestamp = 0;
    if (!SourceInfo(sourcePath, sourceSize, sourceTimestamp))
        return false;

    std::ifstream in(cookedPath, std::ios::binary);
    if (!in)
        return false;

    CookedModelHeader header;
    Reader reader(in);
    if (!reader.Value(header))
        return false;

    return header.magic == CookedModelMagic &&
           header.version == CookedModelVersion &&
           header.sourceSize == sourceSize &&
           header.sourceTimestamp == sourceTimestamp;
}

} // namespace Lgt::Assets
