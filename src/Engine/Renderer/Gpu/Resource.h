#pragma once
#include <cstdint>
#include <memory>
#include <string>

#include "Engine/Renderer/Vulkan/Helpers.h"

#include "Engine/Core/Logger.h"
#include "Engine/Core/Core.h"

namespace Lgt::Gpu {

struct Extent {
    uint32_t width  = 0;
    uint32_t height = 0;
};

struct Vertex {
    glm::vec3 position;
    uint32_t  pad0;
    glm::vec3 normal;
    uint32_t  pad1;
    glm::vec2 uv;
    uint32_t  pad3[2];
    glm::vec4 tangent;
};

struct Buffer {
    VkBuffer        buffer = VK_NULL_HANDLE;
    void*           mapped = nullptr;
    VkDeviceAddress deviceAddress;
    VmaAllocation   allocation = VK_NULL_HANDLE;
    // VmaAllocationInfo allocInfo  = {};
    VkDeviceSize size = 0;
    std::string  debugName;
};

struct Texture {
    VkImage              image       = VK_NULL_HANDLE;
    VkImageView          defaultView = VK_NULL_HANDLE;
    VmaAllocation        allocation  = VK_NULL_HANDLE;
    VkFormat             format      = VK_FORMAT_UNDEFINED;
    Extent               extent;
    uint32_t             mipLevels        = 1;
    uint32_t             arrayLayers      = 1;
    bool                 isSwapchainImage = false;
    VkImageLayout        currentLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags        currentAccess    = 0;
    VkPipelineStageFlags currentStage     = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    std::string          debugName;
};

struct TextureViewDesc {
    VkImageViewCreateInfo info = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0,
        .image    = VK_NULL_HANDLE,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = VK_FORMAT_UNDEFINED,
        .components = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        .subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount     = VK_REMAINING_ARRAY_LAYERS,
        },
    };

    // ── Common factory methods ──────────────────────────────────────────

    static TextureViewDesc ShaderResource2D(VkFormat format,
                                            VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT) {
        TextureViewDesc desc;
        desc.info.viewType                       = VK_IMAGE_VIEW_TYPE_2D;
        desc.info.format                         = format;
        desc.info.subresourceRange.aspectMask     = aspect;
        desc.info.subresourceRange.baseMipLevel   = 0;
        desc.info.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
        desc.info.subresourceRange.baseArrayLayer = 0;
        desc.info.subresourceRange.layerCount     = 1;
        return desc;
    }

    static TextureViewDesc DepthStencil(VkFormat format = VK_FORMAT_D32_SFLOAT) {
        TextureViewDesc desc;
        desc.info.viewType                       = VK_IMAGE_VIEW_TYPE_2D;
        desc.info.format                         = format;
        desc.info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        desc.info.subresourceRange.baseMipLevel   = 0;
        desc.info.subresourceRange.levelCount     = 1;
        desc.info.subresourceRange.baseArrayLayer = 0;
        desc.info.subresourceRange.layerCount     = 1;
        return desc;
    }

    static TextureViewDesc StorageImage(VkFormat format) {
        TextureViewDesc desc;
        desc.info.viewType                       = VK_IMAGE_VIEW_TYPE_2D;
        desc.info.format                         = format;
        desc.info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        desc.info.subresourceRange.baseMipLevel   = 0;
        desc.info.subresourceRange.levelCount     = 1;
        desc.info.subresourceRange.baseArrayLayer = 0;
        desc.info.subresourceRange.layerCount     = 1;
        return desc;
    }

    static TextureViewDesc CubeMap(VkFormat format) {
        TextureViewDesc desc;
        desc.info.viewType                       = VK_IMAGE_VIEW_TYPE_CUBE;
        desc.info.format                         = format;
        desc.info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        desc.info.subresourceRange.baseMipLevel   = 0;
        desc.info.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
        desc.info.subresourceRange.baseArrayLayer = 0;
        desc.info.subresourceRange.layerCount     = 6;
        return desc;
    }

    static TextureViewDesc ArrayLayer(VkFormat format, uint32_t layer, uint32_t mipLevels = VK_REMAINING_MIP_LEVELS) {
        TextureViewDesc desc;
        desc.info.viewType                       = VK_IMAGE_VIEW_TYPE_2D;
        desc.info.format                         = format;
        desc.info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        desc.info.subresourceRange.baseMipLevel   = 0;
        desc.info.subresourceRange.levelCount     = mipLevels;
        desc.info.subresourceRange.baseArrayLayer = layer;
        desc.info.subresourceRange.layerCount     = 1;
        return desc;
    }

    static TextureViewDesc MipLevel(VkFormat format, uint32_t mip) {
        TextureViewDesc desc;
        desc.info.viewType                       = VK_IMAGE_VIEW_TYPE_2D;
        desc.info.format                         = format;
        desc.info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        desc.info.subresourceRange.baseMipLevel   = mip;
        desc.info.subresourceRange.levelCount     = 1;
        desc.info.subresourceRange.baseArrayLayer = 0;
        desc.info.subresourceRange.layerCount     = 1;
        return desc;
    }
};

LGT_DEFINE_HANDLE(Buffer);
LGT_DEFINE_HANDLE(Texture);

// Resource Pool Template
template <typename ResourceType, typename Handle> class ResourcePool {
public:
    using ValueType = uint32_t;

    ResourcePool() {
        _resources.emplace_back();
        _generations.emplace_back(1);
        _alive.emplace_back(false);
    }

    Handle Allocate(ResourceType resource) {
        ValueType index;
        Handle    handle;

        if (_freelist.empty()) {
            index = static_cast<ValueType>(_resources.size());
            _resources.push_back(resource);
            _generations.push_back(1);
            _alive.push_back(true);
        } else {
            index = _freelist.back();
            _freelist.pop_back();
            _resources[index] = resource;
            ++_generations[index];
            _alive[index] = true;
        }

        uint64_t raw = (static_cast<uint64_t>(_generations[index]) << 32) | static_cast<uint64_t>(index);

        handle.value = raw;
        return handle;
    }

    void Free(Handle& handle) {
        LGT_ASSERT(handle.IsValid(), "ResourcePool::free: trying to free an invalid handle");

        uint64_t raw   = handle.value;
        auto     index = static_cast<ValueType>(raw & 0xFFFFFFFF);
        auto     gen   = static_cast<ValueType>(raw >> 32);

        LGT_ASSERT(index < _resources.size() && _generations[index] == gen,
                   "ResourcePool::free: stale or foreign handle detected");

        handle.value      = 0;
        _resources[index] = ResourceType{};
        _alive[index]     = false;
        _freelist.push_back(index);
    }

    ResourceType* Get(const Handle& handle) {
        if (!handle.IsValid()) {
            LIGHTVK_WARN("ResourcePool::get : invalid handle");
            return nullptr;
        }

        uint64_t raw   = handle.value;
        auto     index = static_cast<ValueType>(raw & 0xFFFFFFFF);
        auto     gen   = static_cast<ValueType>(raw >> 32);

        if (!(index < _resources.size() && _generations[index] == gen)) {
            LIGHTVK_WARN("stale or foreign handle detected");
            return nullptr;
        }

        return &_resources[index];
    }

    template <typename Fn> void ForEach(Fn&& fn) {
        for (size_t i = 1; i < _resources.size(); ++i) {
            if (_generations[i] != 0) {
                fn(_resources[i]);
            }
        }
    }

    template <typename Fn> void ForEachAlive(Fn&& fn) {
        for (ValueType i = 1; i < _resources.size(); ++i) {
            if (!_alive[i])
                continue;

            uint32_t gen = _generations[i];

            uint64_t raw = (static_cast<uint64_t>(gen) << 32) | static_cast<uint64_t>(i);

            Handle handle;
            handle.value = raw;

            fn(_resources[i], handle);
        }
    }

    bool IsAlive(const Handle& handle) const {
        if (!handle.IsValid())
            return false;

        uint64_t raw   = handle.id;
        auto     index = static_cast<ValueType>(raw & 0xFFFFFFFF);
        auto     gen   = static_cast<ValueType>(raw >> 32);

        return index < _resources.size() && _generations[index] == gen;
    }

    void Clear() {
        _resources.clear();
        _generations.clear();
        _freelist.clear();
        _alive.clear();
        _resources.emplace_back();
        _generations.emplace_back(1);
        _alive.emplace_back(false);
    }

private:
    std::vector<ResourceType> _resources;
    std::vector<ValueType>    _generations;
    std::vector<bool>         _alive;
    std::vector<ValueType>    _freelist;
};

// TextureHandle createTexture();

} // namespace Lgt::Gpu
