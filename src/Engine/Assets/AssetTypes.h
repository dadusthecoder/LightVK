#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Lgt::Assets {

constexpr uint32_t InvalidIndex = 0xFFFFFFFFu;

struct AssetGuid {
    uint64_t high = 0;
    uint64_t low  = 0;

    bool IsValid() const { return high != 0 || low != 0; }
    friend bool operator==(const AssetGuid&, const AssetGuid&) = default;
};

enum class AssetType : uint8_t {
    Unknown = 0,
    Model,
    Mesh,
    Texture,
    Sampler,
    Material,
};

enum class AssetState : uint8_t {
    Unloaded = 0,
    Loading,
    Imported,
    CpuReady,
    UploadQueued,
    Resident,
    Failed,
};

struct AssetRef {
    AssetGuid id{};
    AssetType type = AssetType::Unknown;

    bool IsValid() const { return id.IsValid() && type != AssetType::Unknown; }
};

struct Float2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Float3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Float4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

struct Transform {
    Float3 position{0.0f, 0.0f, 0.0f};
    Float4 rotation{0.0f, 0.0f, 0.0f, 1.0f};
    Float3 scale{1.0f, 1.0f, 1.0f};
};

struct Bounds {
    Float3 min{};
    Float3 max{};
};

struct MeshVertex {
    Float3 position;
    float positionPadding = 0.0f;
    Float3 normal;
    float normalPadding = 0.0f;
    Float2 uv0;
    float uvPadding[2]{0.0f, 0.0f};
    Float4 tangent;
};

static_assert(sizeof(MeshVertex) == 64, "MeshVertex must match the descriptor-heap shader layout");

struct MeshPrimitive {
    uint32_t vertexOffset = 0;
    uint32_t vertexCount  = 0;
    uint32_t indexOffset  = 0;
    uint32_t indexCount   = 0;
    uint32_t materialIndex = InvalidIndex;
    Bounds bounds{};
};

struct MeshAsset {
    AssetGuid id{};
    std::string name;

    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<MeshPrimitive> primitives;
    Bounds bounds{};
};

enum class TextureSemantic : uint8_t {
    Unknown = 0,
    BaseColor,
    Normal,
    MetallicRoughness,
    Occlusion,
    Emissive,
};

enum class TextureColorSpace : uint8_t {
    Linear = 0,
    SRGB,
};

enum class TextureFormat : uint8_t {
    RGBA8 = 0,
};

struct TextureSlot {
    AssetRef texture{};
    AssetRef sampler{};
    uint32_t uvSet = 0;
    float strength = 1.0f;
    
    bool IsValid() const { return texture.IsValid(); }
};

struct TextureAsset {
    AssetGuid id{};
    std::string name;

    TextureSemantic semantic = TextureSemantic::Unknown;
    TextureColorSpace colorSpace = TextureColorSpace::Linear;
    TextureFormat format = TextureFormat::RGBA8;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    std::vector<uint8_t> data;
};

enum class SamplerFilter : uint8_t {
    Nearest = 0,
    Linear,
};

enum class SamplerMipmapMode : uint8_t {
    Nearest = 0,
    Linear,
};

enum class SamplerAddressMode : uint8_t {
    Repeat = 0,
    MirroredRepeat,
    ClampToEdge,
};

struct SamplerAsset {
    AssetGuid id{};
    std::string name;

    SamplerFilter magFilter = SamplerFilter::Linear;
    SamplerFilter minFilter = SamplerFilter::Linear;
    SamplerMipmapMode mipmapMode = SamplerMipmapMode::Linear;

    SamplerAddressMode addressModeU = SamplerAddressMode::Repeat;
    SamplerAddressMode addressModeV = SamplerAddressMode::Repeat;
    SamplerAddressMode addressModeW = SamplerAddressMode::Repeat;
};

enum class AlphaMode : uint8_t {
    Opaque = 0,
    Mask,
    Blend,
};

struct MaterialAsset {
    AssetGuid id{};
    std::string name;

    Float4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
    Float3 emissiveFactor{};
    float metallicFactor = 0.0f;
    float roughnessFactor = 1.0f;
    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;
    float emissiveStrength = 1.0f;
    float alphaCutoff = 0.5f;

    TextureSlot baseColor;
    TextureSlot normal;
    TextureSlot metallicRoughness;
    TextureSlot occlusion;
    TextureSlot emissive;

    AlphaMode alphaMode = AlphaMode::Opaque;
    bool doubleSided = false;
};

enum class Mobility : uint8_t {
    Static = 0,
    Movable,
    Skinned,
};

struct NodeMetadata {
    Mobility mobility = Mobility::Static;
    bool castShadow = true;
    bool receiveShadow = true;
    std::string role;
};

struct ModelNode {
    std::string name;
    uint32_t parentIndex = InvalidIndex;
    uint32_t meshIndex = InvalidIndex;
    Transform localTransform{};
    NodeMetadata metadata{};
};

struct ModelAsset {
    AssetGuid id{};
    std::string name;

    std::vector<ModelNode> nodes;
    std::vector<MeshAsset> meshes;
    std::vector<MaterialAsset> materials;
    std::vector<TextureAsset> textures;
    std::vector<SamplerAsset> samplers;
};

} // namespace Lgt::Assets
