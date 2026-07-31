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

using PassExecuteFn = void (*)(void* userdata);

struct RenderGraphNodePass {

    std::string                name;
    PassExecuteFn              execute;
    std::vector<TextureHandle> readTextures;
    std::vector<TextureHandle> writeTextures;
    std::vector<BufferHandle>  readBuffer;
    std::vector<BufferHandle>  writeBuffer;
};

class RenderGraph {

    void AddPass(std::string name);

private:
    std::vector<RenderGraphNodePass> graph_compiled_;
    std::vector<RenderGraphNodePass> graph_intenal_;
};
} // namespace Lgt::Gpu