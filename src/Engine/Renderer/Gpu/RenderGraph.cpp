#include "RenderGraph.h"

namespace Lgt::Gpu {
void RenderGraphClass::Init() {
    LIGHTVK_INFO("RenderGraph Initialized");
}

void RenderGraphClass::ShoutDown() {
    LIGHTVK_INFO("RenderGraph Shutting Down");
}

// i used this design cuse it gives me access to the node and pass with same id in o(1) time complexcity
// while keeping the pass and nodes decoupled
// later i tend to update this solution
// TODO - find a better solution

void RenderGraphClass::AddPass(RenderGraphPass pass) {
    pass.passID = static_cast<uint32_t>(passes_.size());
    passes_.push_back(std::move(pass));
    RenderGraphNode node;
    node.passID = pass.passID;
    nodes_.push_back(std::move(node));
}

void RenderGraphClass::Execute() {}

// the current implementions dose not supprot the multimple p[roducer ,
// so i am just silently continuing to the last writer and usiing it as the producer
// which results in the undefined behaviour for now .so i the engine hard asserts in the debug build

void RenderGraphClass::CollectResources() {
    for (auto& pass : passes_) {
        for (auto& resource : pass.resources) {
            ResourceKey key{.type = resource.type, .handle = resource.handle};
            if (resource.access == Access::Write) {
                LGT_ASSERT(resources_[key].producer != UINT32_MAX,
                           "Multiple Writes to the same resource in pass {} , resource {}",
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
    for (auto& pass : passes_) {

        RenderGraphNode node;
        node.passID = pass.passID;

        for (auto& resource : pass.resources) {

            if (resource.access == Access::Read)
                node.indegree++;
            else {
                ResourceKey resource_key{.type = resource.type, .handle = resource.handle};
                for (auto& child : resources_[resource_key].consumers)
                    node.childern.push_back(child);
            }
        }

        nodes_.push_back(std::move(node));
    }

    for (auto& resource : resources_) {
        for (auto& consumer : resource.second.consumers) {
            edges_.push_back({consumer, resource.second.producer, resource.first.type, resource.first.handle});
        }
    }
}

void RenderGraphClass::Validate() {
    for (auto& edge : edges_) {

        LIGHTVK_INFO("Rendergraph Edge : p : {} , C : {} , RT : {} , RH : {}",
                     edge.producer,
                     edge.consumer,
                     (uint8_t)edge.type,
                     edge.handle);

        LGT_ASSERT(edge.producer != UINT32_MAX, "Invalid RenderGraph");
    }
}

void RenderGraphClass::Sort() {

    std::vector<uint32_t> queue;

    for (auto& node : nodes_) {
        if (node.indegree == 0)
            queue.push_back(node.passID);
    }

    // kahn's algorithm , bfs sort
    while (!queue.empty()) {
        auto nodeID = queue.back();
        execution_.push_back(nodeID);
        queue.pop_back();
        for (auto& childID : nodes_[nodeID].childern) {
            if (nodes_[childID].indegree == 0 || --nodes_[childID].indegree == 0)
                queue.push_back(childID);
        }
    }
}

void RenderGraphClass::Compile() {
    edges_.clear();
    resources_.clear();

    CollectResources();
    ResolveDependencies();
    Validate();
    Sort();
}

} // namespace Lgt::Gpu
