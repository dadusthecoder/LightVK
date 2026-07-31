#include "RenderGraph.h"

namespace Lgt::Gpu {
void RenderGraphClass::Init() {
    LIGHTVK_INFO("RenderGraph Initialized");
}

void RenderGraphClass::ShoutDown() {
    LIGHTVK_INFO("RenderGraph Shutting Down");
}

void RenderGraphClass::AddPass(RenderGraphPassNode pass) {
    graph_intenal_.push_back(pass);
}

void RenderGraphClass::Execute() {}

} // namespace Lgt::Gpu
