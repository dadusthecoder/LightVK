#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Engine/Core/Logger.h"
#include "Engine/Renderer/Vulkan/Helpers.h"
#include "Engine/Renderer/Gpu/Resource.h"

namespace Lgt::Gpu { // namespace  Lgt::Gpu

struct RenderGraphPass;

using RenderGraphExecuteCallback = void (*)(RenderGraphPass* pass, void* userdata);

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

struct RenderGraphPass {
    uint32_t                   passID  = UINT32_MAX;
    std::string                name    = "pass";
    RenderGraphExecuteCallback execute = nullptr;

    std::vector<ResourceRef> resources;

    RenderGraphPass& Reads(TextureHandle texture) {
        resources.push_back({texture.value, ResourceKind::Texture, Access::Read});
        return *this;
    }

    RenderGraphPass& Reads(BufferHandle buffer) {
        resources.push_back({buffer.value, ResourceKind::Buffer, Access::Read});
        return *this;
    }

    RenderGraphPass& Writes(TextureHandle texture) {
        resources.push_back({texture.value, ResourceKind::Texture, Access::Write});
        return *this;
    }

    RenderGraphPass& Writes(BufferHandle buffer) {
        resources.push_back({buffer.value, ResourceKind::Buffer, Access::Write});
        return *this;
    }

    RenderGraphPass& SetExecute(RenderGraphExecuteCallback callback) {
        execute = callback;
        return *this;
    }
};

struct RenderGraphNode {
    uint32_t                     indegree = 0;
    uint32_t                     passID   = UINT32_MAX;
    std::unordered_set<uint32_t> childern;
};

struct ResourceInfo {
    uint32_t              producer = UINT32_MAX;
    std::vector<uint32_t> consumers;
};

// later this graph may consume passes that can be executed parally , or can be recorded parraly on the cpu
// this is just a barebone implemention of what i am trying to build;

class RenderGraphClass {
public:
    void Init();
    void ShoutDown();
    //  for now this graph only supports one writer per resource
    //  later memory aliasing will be added to solve this problem

    // TODO - Memory aliasing
    // Graph Validation
    //

    void AddPass(RenderGraphPass);
    void Compile();
    void Execute();

private:
    void CollectResources();
    void ResolveDependencies();
    void Validate(); // not quite right
    void TopologicalSort();

    std::vector<RenderGraphPass> passes_;
    std::vector<RenderGraphNode> nodes_;
    std::vector<uint32_t>        execution_;

    std::unordered_map<ResourceKey, ResourceInfo> resources_;
};
} // namespace Lgt::Gpu