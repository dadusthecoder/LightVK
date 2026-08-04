#include "RenderGraph.h"

namespace Lgt::Gpu {
void RenderGraphClass::Init() {
    LIGHTVK_INFO("RenderGraph Initialized");
}

void RenderGraphClass::ShoutDown() {
    LIGHTVK_INFO("RenderGraph Shutting Down");
}

void RenderGraphClass::AddPass(RenderGraphPass pass) {
    pass.passID = static_cast<uint32_t>(passes_.size());
    LGT_ASSERT(pass.execute, "pass {} has no execution callback", pass.name);
    passes_.push_back(std::move(pass));
}

void RenderGraphClass::Execute() {
    for (auto passID : execution_) {
        auto& pass = passes_[passID];
        pass.execute(&pass, nullptr);
    }
}

// the current implementions dose not supprot the multimple producer ,
// which results in the undefined behaviour for now .so the engine asserts in the debug build
void RenderGraphClass::CollectResources() {
    for (auto& pass : passes_) {

        RenderGraphNode node;
        node.passID = pass.passID;
        nodes_.push_back(std::move(node));

        for (auto& resource : pass.resources) {
            ResourceKey key{.type = resource.type, .handle = resource.handle};
            if (resource.access == Access::Write) {
                LGT_ASSERT(resources_[key].producer == UINT32_MAX,
                           "Multiple Writes to the same resource in Renderpass {} , resource {}",
                           pass.name,
                           resource.handle);

                resources_[key].producer = pass.passID;
            } else if (resource.access == Access::Read) {
                resources_[key].consumers.push_back(pass.passID);
            } else {
                LIGHTVK_ERROR("Resource {} has no access defined", resource.handle);
            }
        }
    }
}

void RenderGraphClass::ResolveDependencies() {
    for (auto& resource : resources_) {
        for (const auto& consumer : resource.second.consumers) {
            if (resource.second.producer != UINT32_MAX) {
                auto [iter, inserted] = nodes_[resource.second.producer].childern.insert(consumer);
                // Only increment indegree if this edge didn't exist before
                if (inserted) {
                    nodes_[consumer].indegree++;
                }
            }
        }
    }
}

void RenderGraphClass::Validate() {
    for (const auto& [key, info] : resources_) {
        if (info.producer == UINT32_MAX) {
            LIGHTVK_WARN("Resource (type: {}, handle: {}) has no producer. Assuming external resource.", 
                         (uint32_t)key.type, key.handle);
        }
        
        if (info.consumers.empty()) {
            if (info.producer != UINT32_MAX) {
                LIGHTVK_WARN("Resource (type: {}, handle: {}) is produced by pass '{}' but never consumed.", 
                             (uint32_t)key.type, key.handle, passes_[info.producer].name);
            }
        } else {
            for (auto consumer : info.consumers) {
                LIGHTVK_TRACE("RenderGraph Edge: Producer: {} -> Consumer: {} | Resource: (type: {}, handle: {})",
                              info.producer != UINT32_MAX ? passes_[info.producer].name : "External",
                              passes_[consumer].name,
                              (uint32_t)key.type, key.handle);
            }
        }
    }
}

void RenderGraphClass::TopologicalSort() {

    std::vector<uint32_t> queue;
    size_t                head      = 0;
    uint32_t              processed = 0;

    for (auto& node : nodes_) {
        if (node.indegree == 0)
            queue.push_back(node.passID);
    }

    // kahn's algorithm , bfs
    while (head < queue.size()) {
        auto nodeID = queue[head++];
        execution_.push_back(nodeID);
        processed++;
        for (auto& childID : nodes_[nodeID].childern) {
            // this statements directly mutates the graph
            if (--nodes_[childID].indegree == 0)
                queue.push_back(childID);
        }
    }

    if (execution_.size() != passes_.size()) {
        LIGHTVK_ERROR("RenderGraph Cycle Detected! Execution pipeline aborted.");
        execution_.clear();
    }
}

void RenderGraphClass::Compile() {
    resources_.clear();
    execution_.clear();
    nodes_.clear();

    CollectResources();
    ResolveDependencies();
    Validate();
    TopologicalSort();
}

} // namespace Lgt::Gpu
