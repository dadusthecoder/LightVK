#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "Engine/Core/Logger.h"
#include "Engine/Renderer/Vulkan/Helpers.h"
#include "Engine/Renderer/Gpu/Resource.h"

namespace Lgt::Gpu { // namespace  Lgt::Gpu

using RenderGraphExecuteCallback = std::function<void(VkCommandBuffer)>;

enum class Access : uint8_t {
    Read,
    Write
};

enum class ResourceKind : uint8_t {
    Texture,
    Buffer
};

struct ResourceRef {
    uint64_t     handle;
    ResourceKind type;
    Access       access;
};

struct ResourceKey {
    ResourceKind type;
    uint64_t     handle;

    bool operator==(const ResourceKey&) const = default;
};

} // namespace Lgt::Gpu

namespace std {
template <> struct hash<Lgt::Gpu::ResourceKey> {
    size_t operator()(const Lgt::Gpu::ResourceKey& r) const {
        return hash<uint64_t>()(r.handle) ^ (hash<int>()((int)r.type) << 1);
    }
};
} // namespace std

namespace Lgt::Gpu {

struct ResourceInfo {
    uint32_t              producer = -1;
    std::vector<uint32_t> consumers;
};

struct RenderGraphNode {
    uint32_t                   passID  = UINT32_MAX;
    std::string                name    = "pass";
    RenderGraphExecuteCallback execute = nullptr;

    std::vector<ResourceRef> resources;

    RenderGraphNode& Reads(TextureHandle texture) {
        resources.push_back({texture.value, ResourceKind::Texture, Access::Read});
        return *this;
    }

    RenderGraphNode& Reads(BufferHandle buffer) {
        resources.push_back({buffer.value, ResourceKind::Buffer, Access::Read});
        return *this;
    }

    RenderGraphNode& Writes(TextureHandle texture) {
        resources.push_back({texture.value, ResourceKind::Texture, Access::Write});
        return *this;
    }

    RenderGraphNode& Writes(BufferHandle buffer) {
        resources.push_back({buffer.value, ResourceKind::Buffer, Access::Write});
        return *this;
    }
};

struct RenderGraphEdge {
    uint32_t     consumer = -1;
    uint32_t     producer = -1;
    ResourceKind type;
    uint64_t     handle;
};

// later this graph may consume passes that can be executed parally , or can be recorded parraly on the cpu
// this is just a barebone implemention of what i am trying to build;

class RenderGraphClass {
public:
    void Init();
    void ShoutDown();
    void AddPass(RenderGraphNode);
    void Compile();
    void Execute();

private:
    void                                          CollectResources();
    void                                          ResolveDependencies();
    void                                          Validate();
    
    std::vector<RenderGraphEdge>                  edges_;
    std::vector<RenderGraphNode>                  nodes_;
    std::unordered_map<ResourceKey, ResourceInfo> resources_;
};
} // namespace Lgt::Gpu