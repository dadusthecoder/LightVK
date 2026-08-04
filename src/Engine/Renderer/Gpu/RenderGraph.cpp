#include "RenderGraph.h"

namespace Lgt::Gpu {
void RenderGraphClass::Init() {
    LIGHTVK_INFO("RenderGraph Initialized");
}

void RenderGraphClass::ShoutDown() {
    LIGHTVK_INFO("RenderGraph Shutting Down");
}

void RenderGraphClass::AddPass(RenderGraphPass pass) {
    pass.passID = static_cast<uint32_t>(_passes.size());
    LGT_ASSERT(pass.execute, "pass {} has no execution callback", pass.name);
    _passes.push_back(std::move(pass));
}

void RenderGraphClass::Execute() {
    for (auto passID : _execution) {
        auto& pass = _passes[passID];
        pass.execute(&pass, nullptr);
    }
}

// the current implementions dose not supprot the multimple producer ,
// which results in the undefined behaviour for now .so the engine asserts in the debug build
void RenderGraphClass::CollectResources() {
    for (auto& pass : _passes) {

        RenderGraphNode node;
        node.passID = pass.passID;
        _nodes.push_back(std::move(node));

        for (auto& resource : pass.resources) {
            ResourceKey key{.type = resource.type, .handle = resource.handle};
            if (resource.access == Access::Write) {
                LGT_ASSERT(_resources[key].producer == UINT32_MAX,
                           "Multiple Writes to the same resource in Renderpass {} , resource {}",
                           pass.name,
                           resource.handle);

                _resources[key].producer = pass.passID;
            } else if (resource.access == Access::Read) {
                _resources[key].consumers.push_back(pass.passID);
            } else {
                LIGHTVK_ERROR("Resource {} has no access defined", resource.handle);
            }
        }
    }
}

void RenderGraphClass::ResolveDependencies() {
    for (auto& resource : _resources) {
        for (const auto& consumer : resource.second.consumers) {
            if (resource.second.producer != UINT32_MAX) {
                auto [iter, inserted] = _nodes[resource.second.producer].childern.insert(consumer);
                // Only increment indegree if this edge didn't exist before
                if (inserted) {
                    _nodes[consumer].indegree++;
                }
            }
        }
    }
}

void RenderGraphClass::Validate() {
    for (const auto& [key, info] : _resources) {
        if (info.producer == UINT32_MAX) {
            LIGHTVK_WARN(
                "Resource (type: {}, handle: {}) has no producer. Assuming external resource.", (uint32_t)key.type, key.handle);
        }

        if (info.consumers.empty()) {
            if (info.producer != UINT32_MAX) {
                LIGHTVK_WARN("Resource (type: {}, handle: {}) is produced by pass '{}' but never consumed.",
                             (uint32_t)key.type,
                             key.handle,
                             _passes[info.producer].name);
            }
        } else {
            for (auto consumer : info.consumers) {
                LIGHTVK_TRACE("RenderGraph Edge: Producer: {} -> Consumer: {} | Resource: (type: {}, handle: {})",
                              info.producer != UINT32_MAX ? _passes[info.producer].name : "External",
                              _passes[consumer].name,
                              (uint32_t)key.type,
                              key.handle);
            }
        }
    }
}

void RenderGraphClass::TopologicalSort() {

    std::vector<uint32_t> queue;
    size_t                head      = 0;
    uint32_t              processed = 0;

    for (auto& node : _nodes) {
        if (node.indegree == 0)
            queue.push_back(node.passID);
    }

    // kahn's algorithm , bfs
    while (head < queue.size()) {
        auto nodeID = queue[head++];
        _execution.push_back(nodeID);
        processed++;
        for (auto& childID : _nodes[nodeID].childern) {
            // this statements directly mutates the graph
            if (--_nodes[childID].indegree == 0)
                queue.push_back(childID);
        }
    }

    if (_execution.size() != _passes.size()) {
        LIGHTVK_ERROR("RenderGraph Cycle Detected! Execution pipeline aborted.");
        _execution.clear();
    }
}

void RenderGraphClass::Compile() {
    _resources.clear();
    _execution.clear();
    _nodes.clear();

    CollectResources();
    ResolveDependencies();
    Validate();
    TopologicalSort();
}

} // namespace Lgt::Gpu
