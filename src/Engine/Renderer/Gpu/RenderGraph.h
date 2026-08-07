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

class RenderGraphBuilder;
struct RenderGraphPass;

using RenderGraphSetupCallback   = void (*)(const RenderGraphBuilder& builder, RenderGraphPass* pass);
using RenderGraphExecuteCallback = void (*)(RenderGraphPass* pass, void* userdata);

class RenderGraphBuilder {
public:
    TextureHandle CreateTexture(...);
    BufferHandle  CreateBuffer(...);
    void          Read(TextureHandle);
    void          Write(TextureHandle);
    void          Import(TextureHandle);
    void          Export(TextureHandle);
};

struct RenderGraphPass {
    uint32_t                   passID  = UINT32_MAX;
    std::string                name    = "pass";
    RenderGraphExecuteCallback execute = nullptr;
    RenderGraphSetupCallback   setup   = nullptr;

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

class RenderGraphClass {
public:
    void Init();
    void ShoutDown();
    void Reset();

    // TODO
    // 2) Barrier Generation

    // 1) Memory aliasing

    // 2) support multiple writers to the same resource
    /*
      for eg. shadow bloom reads HDR and then Writes HDR which is perfectly valid but the current implemention assert on
      such case .
    */

    // 3) imported resources

    template <typename T> void AddPass() {
        RenderGraphPass pass;
        pass.passID  = static_cast<uint32_t>(passes_.size());
        pass.execute = &T::Execute;
        pass.setup   = &T::Setup;
        passes_.push_back(std::move(pass));
    };

    void Compile();
    void Execute();

private:
    void CollectResources();
    void ResolveDependencies();
    void Validate();
    void TopologicalSort();

    std::vector<RenderGraphPass> passes_;
    std::vector<RenderGraphNode> nodes_;
    std::vector<uint32_t>        execution_;

    std::unordered_map<ResourceKey, ResourceInfo> resources_;
};
} // namespace Lgt::Gpu
