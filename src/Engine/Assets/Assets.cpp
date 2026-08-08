#include "Assets.h"

#include "Engine/Core/Logger.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

namespace Lgt::Assets {
namespace {

AssetGuid MakeGuid(std::string_view seed) {
    uint64_t a = 1469598103934665603ull;
    uint64_t b = 1099511628211ull;

    for (unsigned char c : seed) {
        a ^= c;
        a *= 1099511628211ull;
        b += c + (b << 6u) + (b >> 2u);
        b ^= (a >> 17u);
    }

    if (a == 0 && b == 0)
        b = 1;
    return {a, b};
}

Float3 ToFloat3(const fastgltf::math::fvec3& value) {
    return {value.x(), value.y(), value.z()};
}

Float4 ToFloat4(const fastgltf::math::fvec4& value) {
    return {value.x(), value.y(), value.z(), value.w()};
}

Transform ToTransform(const fastgltf::Node& node) {
    Transform result{};

    if (const auto* trs = std::get_if<fastgltf::TRS>(&node.transform)) {
        result.position = ToFloat3(trs->translation);
        result.rotation = {trs->rotation.x(), trs->rotation.y(), trs->rotation.z(), trs->rotation.w()};
        result.scale = ToFloat3(trs->scale);
    } else {
        // Matrix nodes are only supported losslessly when the parser decomposes them.
        // ImportGltf requests that option below.
        result.position = {0.0f, 0.0f, 0.0f};
        result.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        result.scale = {1.0f, 1.0f, 1.0f};
    }

    return result;
}

SamplerFilter ToFilter(std::optional<fastgltf::Filter> filter) {
    if (!filter.has_value())
        return SamplerFilter::Linear;

    return *filter == fastgltf::Filter::Nearest ? SamplerFilter::Nearest : SamplerFilter::Linear;
}

SamplerMipmapMode ToMipmapMode(std::optional<fastgltf::Filter> filter) {
    if (!filter.has_value())
        return SamplerMipmapMode::Linear;

    switch (*filter) {
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::LinearMipMapNearest:
        return SamplerMipmapMode::Nearest;
    default:
        return SamplerMipmapMode::Linear;
    }
}

SamplerAddressMode ToAddressMode(fastgltf::Wrap mode) {
    switch (mode) {
    case fastgltf::Wrap::ClampToEdge:
        return SamplerAddressMode::ClampToEdge;
    case fastgltf::Wrap::MirroredRepeat:
        return SamplerAddressMode::MirroredRepeat;
    default:
        return SamplerAddressMode::Repeat;
    }
}

std::vector<uint8_t> ReadFileBytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        return {};

    const auto size = file.tellg();
    if (size <= 0)
        return {};

    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return file ? bytes : std::vector<uint8_t>{};
}

bool DecodeImage(const std::vector<uint8_t>& encoded,
                 TextureAsset* texture,
                 TextureColorSpace colorSpace,
                 TextureSemantic semantic) {
    if (encoded.empty() || texture == nullptr)
        return false;

    int width = 0;
    int height = 0;
    int sourceChannels = 0;
    constexpr int decodedChannels = 4;

    stbi_uc* decoded = stbi_load_from_memory(encoded.data(),
                                             static_cast<int>(encoded.size()),
                                             &width,
                                             &height,
                                             &sourceChannels,
                                             decodedChannels);
    if (decoded == nullptr)
        return false;

    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> image(decoded, &stbi_image_free);

    if (width <= 0 || height <= 0)
        return false;

    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (pixelCount > std::numeric_limits<size_t>::max() / decodedChannels)
        return false;

    texture->width = static_cast<uint32_t>(width);
    texture->height = static_cast<uint32_t>(height);
    texture->mipLevels = 1;
    texture->format = TextureFormat::RGBA8;
    texture->colorSpace = colorSpace;
    texture->semantic = semantic;
    texture->data.assign(image.get(), image.get() + pixelCount * decodedChannels);
    return true;
}

std::vector<uint8_t> ImageBytes(const fastgltf::Asset& asset,
                                const fastgltf::Image& image,
                                const std::filesystem::path& basePath) {
    return std::visit(fastgltf::visitor{
                          [](const std::monostate&) { return std::vector<uint8_t>{}; },
                          [&](const fastgltf::sources::BufferView& source) {
                              fastgltf::DefaultBufferDataAdapter adapter;
                              const auto bytes = adapter(asset, source.bufferViewIndex);
                              return std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(bytes.data()),
                                                          reinterpret_cast<const uint8_t*>(bytes.data()) + bytes.size());
                          },
                          [&](const fastgltf::sources::URI& source) {
                              return ReadFileBytes(basePath / source.uri.fspath());
                          },
                          [](const fastgltf::sources::Array& source) {
                              return std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(source.bytes.data()),
                                                          reinterpret_cast<const uint8_t*>(source.bytes.data()) + source.bytes.size_bytes());
                          },
                          [](const fastgltf::sources::Vector& source) {
                              return std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(source.bytes.data()),
                                                          reinterpret_cast<const uint8_t*>(source.bytes.data()) + source.bytes.size());
                          },
                          [](const fastgltf::sources::ByteView& source) {
                              return std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(source.bytes.data()),
                                                          reinterpret_cast<const uint8_t*>(source.bytes.data()) + source.bytes.size());
                          },
                          [](const fastgltf::sources::CustomBuffer&) { return std::vector<uint8_t>{}; },
                          [](const fastgltf::sources::Fallback&) { return std::vector<uint8_t>{}; },
                      },
                      image.data);
}

TextureSlot MakeTextureSlot(const fastgltf::Asset& asset,
                            const fastgltf::TextureInfo& info,
                            AssetType expectedType,
                            const std::filesystem::path& sourcePath,
                            std::vector<TextureAsset>& textures,
                            const std::vector<SamplerAsset>& samplers,
                            TextureSemantic semantic,
                            TextureColorSpace colorSpace,
                            float strength = 1.0f) {
    TextureSlot slot;
    slot.uvSet = static_cast<uint32_t>(info.texCoordIndex);
    slot.strength = strength;

    if (info.textureIndex >= asset.textures.size())
        return slot;

    const auto& gltfTexture = asset.textures[info.textureIndex];
    if (!gltfTexture.imageIndex.has_value() || *gltfTexture.imageIndex >= asset.images.size())
        return slot;

    const std::string imageSeed = sourcePath.generic_string() + "#image:" + std::to_string(*gltfTexture.imageIndex);
    const AssetGuid imageId = MakeGuid(imageSeed);

    auto existing = std::find_if(textures.begin(), textures.end(), [&](const TextureAsset& texture) {
        return texture.id == imageId;
    });

    if (existing == textures.end()) {
        TextureAsset texture;
        texture.id = imageId;
        texture.name = std::string(asset.images[*gltfTexture.imageIndex].name);
        if (texture.name.empty())
            texture.name = "image_" + std::to_string(*gltfTexture.imageIndex);

        const auto encoded = ImageBytes(asset, asset.images[*gltfTexture.imageIndex], sourcePath.parent_path());
        if (!DecodeImage(encoded, &texture, colorSpace, semantic)) {
            LIGHTVK_WARN("GltfImporter: failed to decode image '{}'", texture.name);
            return slot;
        }
        textures.push_back(std::move(texture));
    }

    slot.texture = {imageId, expectedType};

    if (gltfTexture.samplerIndex.has_value() && *gltfTexture.samplerIndex < samplers.size()) {
        slot.sampler = {samplers[*gltfTexture.samplerIndex].id, AssetType::Sampler};
    } else {
        slot.sampler = {MakeGuid(sourcePath.generic_string() + "#sampler:default"), AssetType::Sampler};
    }

    return slot;
}

} // namespace

AssetGuid AssetIdForPath(const std::filesystem::path& path) {
    return MakeGuid(path.lexically_normal().generic_string());
}

ImportResult ImportGltf(const std::filesystem::path& path, const ImportOptions&) {
    ImportResult result;
    result.model.id = MakeGuid(path.lexically_normal().generic_string());
    result.model.name = path.stem().string();

    fastgltf::Parser parser;
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (data.error() != fastgltf::Error::None) {
        result.error = "Cannot open glTF file: " + path.string();
        return result;
    }

    const auto options = fastgltf::Options::LoadExternalBuffers |
                         fastgltf::Options::LoadExternalImages |
                         fastgltf::Options::GenerateMeshIndices |
                         fastgltf::Options::DecomposeNodeMatrices;
    auto gltf = parser.loadGltf(data.get(), path.parent_path(), options);
    if (gltf.error() != fastgltf::Error::None) {
        result.error = "Failed to parse glTF file: " + path.string();
        return result;
    }

    const auto& asset = gltf.get();

    result.model.samplers.reserve(asset.samplers.size());
    for (size_t i = 0; i < asset.samplers.size(); ++i) {
        const auto& source = asset.samplers[i];
        SamplerAsset sampler;
        sampler.id = MakeGuid(path.generic_string() + "#sampler:" + std::to_string(i));
        sampler.name = std::string(source.name);
        sampler.magFilter = ToFilter(source.magFilter);
        sampler.minFilter = ToFilter(source.minFilter);
        sampler.mipmapMode = ToMipmapMode(source.minFilter);
        sampler.addressModeU = ToAddressMode(source.wrapS);
        sampler.addressModeV = ToAddressMode(source.wrapT);
        sampler.addressModeW = sampler.addressModeV;
        result.model.samplers.push_back(std::move(sampler));
    }

    SamplerAsset defaultSampler;
    defaultSampler.id = MakeGuid(path.generic_string() + "#sampler:default");
    defaultSampler.name = "DefaultSampler";
    result.model.samplers.push_back(std::move(defaultSampler));

    result.model.materials.reserve(asset.materials.size());
    for (size_t i = 0; i < asset.materials.size(); ++i) {
        const auto& source = asset.materials[i];
        MaterialAsset material;
        material.id = MakeGuid(path.generic_string() + "#material:" + std::to_string(i));
        material.name = std::string(source.name);
        material.baseColorFactor = ToFloat4(source.pbrData.baseColorFactor);
        material.metallicFactor = source.pbrData.metallicFactor;
        material.roughnessFactor = source.pbrData.roughnessFactor;
        material.emissiveFactor = ToFloat3(source.emissiveFactor);
        material.emissiveStrength = source.emissiveStrength;
        material.alphaCutoff = source.alphaCutoff;
        material.doubleSided = source.doubleSided;

        switch (source.alphaMode) {
        case fastgltf::AlphaMode::Mask:
            material.alphaMode = AlphaMode::Mask;
            break;
        case fastgltf::AlphaMode::Blend:
            material.alphaMode = AlphaMode::Blend;
            break;
        default:
            material.alphaMode = AlphaMode::Opaque;
            break;
        }

        if (source.pbrData.baseColorTexture.has_value()) {
            material.baseColor = MakeTextureSlot(asset,
                                                  *source.pbrData.baseColorTexture,
                                                  AssetType::Texture,
                                                  path,
                                                  result.model.textures,
                                                  result.model.samplers,
                                                  TextureSemantic::BaseColor,
                                                  TextureColorSpace::SRGB);
        }
        if (source.pbrData.metallicRoughnessTexture.has_value()) {
            material.metallicRoughness = MakeTextureSlot(asset,
                                                         *source.pbrData.metallicRoughnessTexture,
                                                         AssetType::Texture,
                                                         path,
                                                         result.model.textures,
                                                         result.model.samplers,
                                                         TextureSemantic::MetallicRoughness,
                                                         TextureColorSpace::Linear);
        }
        if (source.normalTexture.has_value()) {
            material.normal = MakeTextureSlot(asset,
                                              *source.normalTexture,
                                              AssetType::Texture,
                                              path,
                                              result.model.textures,
                                              result.model.samplers,
                                              TextureSemantic::Normal,
                                              TextureColorSpace::Linear,
                                              source.normalTexture->scale);
        }
        if (source.occlusionTexture.has_value()) {
            material.occlusion = MakeTextureSlot(asset,
                                                 *source.occlusionTexture,
                                                 AssetType::Texture,
                                                 path,
                                                 result.model.textures,
                                                 result.model.samplers,
                                                 TextureSemantic::Occlusion,
                                                 TextureColorSpace::Linear,
                                                 source.occlusionTexture->strength);
        }
        if (source.emissiveTexture.has_value()) {
            material.emissive = MakeTextureSlot(asset,
                                                *source.emissiveTexture,
                                                AssetType::Texture,
                                                path,
                                                result.model.textures,
                                                result.model.samplers,
                                                TextureSemantic::Emissive,
                                                TextureColorSpace::SRGB);
        }

        result.model.materials.push_back(std::move(material));
    }

    result.model.meshes.reserve(asset.meshes.size());
    for (size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex) {
        const auto& sourceMesh = asset.meshes[meshIndex];
        MeshAsset mesh;
        mesh.id = MakeGuid(path.generic_string() + "#mesh:" + std::to_string(meshIndex));
        mesh.name = std::string(sourceMesh.name);

        for (const auto& primitive : sourceMesh.primitives) {
            const auto positionAttribute = primitive.findAttribute("POSITION");
            if (positionAttribute == primitive.attributes.end())
                continue;

            MeshPrimitive importedPrimitive;
            importedPrimitive.vertexOffset = static_cast<uint32_t>(mesh.vertices.size());
            importedPrimitive.indexOffset = static_cast<uint32_t>(mesh.indices.size());
            if (primitive.materialIndex.has_value())
                importedPrimitive.materialIndex = static_cast<uint32_t>(*primitive.materialIndex);

            fastgltf::iterateAccessor<fastgltf::math::fvec3>(
                asset,
                asset.accessors[positionAttribute->accessorIndex],
                [&](const auto& position) {
                    MeshVertex vertex{};
                    vertex.position = ToFloat3(position);
                    vertex.normal = {0.0f, 1.0f, 0.0f};
                    vertex.tangent = {1.0f, 0.0f, 0.0f, 1.0f};
                    mesh.vertices.push_back(vertex);
                });

            importedPrimitive.vertexCount = static_cast<uint32_t>(mesh.vertices.size()) - importedPrimitive.vertexOffset;

            if (const auto normal = primitive.findAttribute("NORMAL"); normal != primitive.attributes.end()) {
                size_t vertexIndex = importedPrimitive.vertexOffset;
                fastgltf::iterateAccessor<fastgltf::math::fvec3>(
                    asset,
                    asset.accessors[normal->accessorIndex],
                    [&](const auto& value) {
                        if (vertexIndex < mesh.vertices.size())
                            mesh.vertices[vertexIndex++].normal = ToFloat3(value);
                    });
            }

            if (const auto uv = primitive.findAttribute("TEXCOORD_0"); uv != primitive.attributes.end()) {
                size_t vertexIndex = importedPrimitive.vertexOffset;
                fastgltf::iterateAccessor<fastgltf::math::fvec2>(
                    asset,
                    asset.accessors[uv->accessorIndex],
                    [&](const auto& value) {
                        if (vertexIndex < mesh.vertices.size())
                            mesh.vertices[vertexIndex++].uv0 = {value.x(), value.y()};
                    });
            }

            if (const auto tangent = primitive.findAttribute("TANGENT"); tangent != primitive.attributes.end()) {
                size_t vertexIndex = importedPrimitive.vertexOffset;
                fastgltf::iterateAccessor<fastgltf::math::fvec4>(
                    asset,
                    asset.accessors[tangent->accessorIndex],
                    [&](const auto& value) {
                        if (vertexIndex < mesh.vertices.size())
                            mesh.vertices[vertexIndex++].tangent = ToFloat4(value);
                    });
            }

            if (primitive.indicesAccessor.has_value()) {
                fastgltf::iterateAccessor<uint32_t>(
                    asset,
                    asset.accessors[*primitive.indicesAccessor],
                    [&](uint32_t index) { mesh.indices.push_back(index + importedPrimitive.vertexOffset); });
            } else {
                for (uint32_t i = 0; i < importedPrimitive.vertexCount; ++i)
                    mesh.indices.push_back(importedPrimitive.vertexOffset + i);
            }

            importedPrimitive.indexCount = static_cast<uint32_t>(mesh.indices.size()) - importedPrimitive.indexOffset;
            mesh.primitives.push_back(importedPrimitive);
        }

        result.model.meshes.push_back(std::move(mesh));
    }

    result.model.nodes.reserve(asset.nodes.size());
    for (size_t nodeIndex = 0; nodeIndex < asset.nodes.size(); ++nodeIndex) {
        const auto& source = asset.nodes[nodeIndex];
        ModelNode node;
        node.name = std::string(source.name);
        node.localTransform = ToTransform(source);
        if (source.meshIndex.has_value())
            node.meshIndex = static_cast<uint32_t>(*source.meshIndex);
        result.model.nodes.push_back(std::move(node));
    }

    for (size_t parentIndex = 0; parentIndex < asset.nodes.size(); ++parentIndex) {
        for (const auto childIndex : asset.nodes[parentIndex].children) {
            if (childIndex < result.model.nodes.size())
                result.model.nodes[childIndex].parentIndex = static_cast<uint32_t>(parentIndex);
        }
    }
 
    result.success = true;
    LIGHTVK_INFO("GltfImporter: '{}' -> {} nodes, {} meshes, {} materials, {} textures, {} samplers",
                 path.filename().string(),
                 result.model.nodes.size(),
                 result.model.meshes.size(),
                 result.model.materials.size(),
                 result.model.textures.size(),
                 result.model.samplers.size());
    return result;
}

} // namespace Lgt::Assets
