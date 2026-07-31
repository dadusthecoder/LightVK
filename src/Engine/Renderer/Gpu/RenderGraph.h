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

using RenderGraphNodePassExecuteFn = void (*)(void* userdata);

struct RenderGraphPassNode {

    std::string                  name    = "pass";
    RenderGraphNodePassExecuteFn execute = nullptr;

    std::vector<TextureHandle> readTextures;
    std::vector<TextureHandle> writeTextures;
    std::vector<BufferHandle>  readBuffers;
    std::vector<BufferHandle>  writeBuffers;

    RenderGraphPassNode Reads(TextureHandle texture) {
        readTextures.push_back(texture);
        return *this;
    }

    RenderGraphPassNode Reads(BufferHandle buffer) {
        readBuffers.push_back(buffer);
        return *this;
    }

    RenderGraphPassNode Writes(TextureHandle texture) {
        writeTextures.push_back(texture);
        return *this;
    }

    RenderGraphPassNode Writes(BufferHandle buffer) {
        writeBuffers.push_back(buffer);
        return *this;
    }
};

class RenderGraphClass {
public:
    void Init();
    void ShoutDown();

    void AddPass(RenderGraphPassNode pass);
    void Execute();

private:
    std::vector<RenderGraphPassNode> graph_compiled_;
    std::vector<RenderGraphPassNode> graph_intenal_;
};
} // namespace Lgt::Gpu