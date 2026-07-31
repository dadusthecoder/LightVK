#pragma once
#include <cstdint>
#include <memory>
#include <string>

#include "Engine/Renderer/Vulkan/Helpers.h"

#include "Engine/Core/Logger.h"
#include "Engine/Core/Core.h"

namespace Lgt::Gpu {

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
    VkImage              image            = VK_NULL_HANDLE;
    VkImageView          defaultView      = VK_NULL_HANDLE;
    VmaAllocation        allocation       = VK_NULL_HANDLE;
    VkFormat             format           = VK_FORMAT_UNDEFINED;
    uint32_t             width            = 0;
    uint32_t             height           = 0;
    uint32_t             mipLevels        = 1;
    uint32_t             arrayLayers      = 1;
    bool                 isSwapchainImage = false;
    VkImageLayout        currentLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags        currentAccess    = 0;
    VkPipelineStageFlags currentStage     = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    std::string          debugName;
};

LGT_DEFINE_HANDLE(Buffer);
LGT_DEFINE_HANDLE(Texture);

// Resource Pool Template
template <typename ResourceType, typename Handle> class ResourcePool {
public:
    using ValueType = uint32_t;

    ResourcePool() {
        resources_.emplace_back();
        generations_.emplace_back(1);
        alive_.emplace_back(false);
    }

    Handle Allocate(ResourceType resource) {
        ValueType index;
        Handle    handle;

        if (freelist_.empty()) {
            index = static_cast<ValueType>(resources_.size());
            resources_.push_back(resource);
            generations_.push_back(1);
            alive_.push_back(true);
        } else {
            index = freelist_.back();
            freelist_.pop_back();
            resources_[index] = resource;
            ++generations_[index];
            alive_[index] = true;
        }

        uint64_t raw = (static_cast<uint64_t>(generations_[index]) << 32) | static_cast<uint64_t>(index);

        handle.id = raw;
        return handle;
    }

    void Free(Handle& handle) {
        LGT_ASSERT(handle.IsValid(), "ResourcePool::free: trying to free an invalid handle");

        uint64_t raw   = handle.id;
        auto     index = static_cast<ValueType>(raw & 0xFFFFFFFF);
        auto     gen   = static_cast<ValueType>(raw >> 32);

        LGT_ASSERT(index < resources_.size() && generations_[index] == gen,
                   "ResourcePool::free: stale or foreign handle detected");

        handle.id         = 0;
        resources_[index] = ResourceType{};
        alive_[index]     = false;
        freelist_.push_back(index);
    }

    ResourceType* Get(const Handle& handle) {
        if (!handle.IsValid()) {
            LIGHTVK_WARN("ResourcePool::get : invalid handle");
            return nullptr;
        }

        uint64_t raw   = handle.id;
        auto     index = static_cast<ValueType>(raw & 0xFFFFFFFF);
        auto     gen   = static_cast<ValueType>(raw >> 32);

        if (!(index < resources_.size() && generations_[index] == gen)) {
            LIGHTVK_WARN("stale or foreign handle detected");
            return nullptr;
        }

        return &resources_[index];
    }

    template <typename Fn> void ForEach(Fn&& fn) {
        for (size_t i = 1; i < resources_.size(); ++i) {
            if (generations_[i] != 0) {
                fn(resources_[i]);
            }
        }
    }

    template <typename Fn> void ForEachAlive(Fn&& fn) {
        for (ValueType i = 1; i < resources_.size(); ++i) {
            if (!alive_[i])
                continue;

            uint32_t gen = generations_[i];

            uint64_t raw = (static_cast<uint64_t>(gen) << 32) | static_cast<uint64_t>(i);

            Handle handle;
            handle.id = raw;

            fn(resources_[i], handle);
        }
    }

    bool IsAlive(const Handle& handle) const {
        if (!handle.IsValid())
            return false;

        uint64_t raw   = handle.id;
        auto     index = static_cast<ValueType>(raw & 0xFFFFFFFF);
        auto     gen   = static_cast<ValueType>(raw >> 32);

        return index < resources_.size() && generations_[index] == gen;
    }

    void Clear() {
        resources_.clear();
        generations_.clear();
        freelist_.clear();
        alive_.clear();
        resources_.emplace_back();
        generations_.emplace_back(1);
        alive_.emplace_back(false);
    }

private:
    std::vector<ResourceType> resources_;
    std::vector<ValueType>    generations_;
    std::vector<bool>         alive_;
    std::vector<ValueType>    freelist_;
};

// TextureHandle createTexture();

} // namespace Lgt::Gpu
