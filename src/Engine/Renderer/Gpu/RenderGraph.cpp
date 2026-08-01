#include "RenderGraph.h"

namespace Lgt::Gpu {
void RenderGraphClass::Init() {
    LIGHTVK_INFO("RenderGraph Initialized");
}

void RenderGraphClass::ShoutDown() {
    LIGHTVK_INFO("RenderGraph Shutting Down");
}

void RenderGraphClass::AddPass(RenderGraphNode pass) {
    pass.passID = static_cast<uint32_t>(nodes_.size());
    nodes_.push_back(std::move(pass));
}

void RenderGraphClass::Execute() {}

void RenderGraphClass::CollectResources() {
    for (auto& node : nodes_) {
        for (auto& resource : node.resources) {
            ResourceKey key{.type = resource.type, .handle = resource.handle};
            if (resource.access == Access::Write) {
                if (resources_[key].producer != -1)
                    LIGHTVK_ERROR("Resource {} Has Multiple Writers", resource.handle);
                resources_[key].producer = node.passID;
            } else if (resource.access == Access::Read) {
                resources_[key].consumers.push_back(node.passID);
            } else {
                LIGHTVK_ERROR("Resource {} has no access defined", resource.handle);
            }
        }
    }
}

void RenderGraphClass::ResolveDependencies() {
    for (auto& resource : resources_) {
        for (auto& consumer : resource.second.consumers) {
            edges_.push_back({consumer, resource.second.producer, resource.first.type, resource.first.handle});
        }
    }
}

void RenderGraphClass::Validate() {
    for (auto& edge : edges_) {
        LGT_ASSERT(edge.producer == -1, "Invalid RenderGraph");
    }
}

void RenderGraphClass::Compile() {
    edges_.clear();

    CollectResources();
    ResolveDependencies();
    Validate();
    
}

} // namespace Lgt::Gpu
