#include "RenderSystem.h"
#include "Engine/Scene/World.h"
#include "Engine/Scene/Components.h"
#include "Engine/Assets/AssetManager.h"

namespace Lgt::System {

Render::Render(World* world, Assets::AssetManager* assets)
    : _world(world), _assets(assets) {}

void Render::SyncAssets() {
    _assets->SyncSceneAssets(_world);
}

Gpu::DrawList Render::BuildDrawList() const {
    Gpu::DrawList result;
    auto view = _world->Registry().view<Component::MeshFilter, 
                                         Component::WorldTransform>();
    
    for (auto [entity, meshFilter, worldTransform] : view.each()) {
        if (auto* renderer = _world->Registry().try_get<Component::MeshRenderer>(entity)) {
            if (!renderer->visible) continue;
        }

        const auto* gpuMesh = _assets->GetGpuMesh(meshFilter.mesh);
        if (!gpuMesh) continue;

        const size_t firstCommand = result.commands.size();
        result.commands.insert(result.commands.end(),
            gpuMesh->drawList.commands.begin(), gpuMesh->drawList.commands.end());
        result.indexCounts.insert(result.indexCounts.end(),
            gpuMesh->drawList.indexCounts.begin(), gpuMesh->drawList.indexCounts.end());

        for (size_t i = firstCommand; i < result.commands.size(); ++i)
            result.commands[i].transform = worldTransform.matrix * result.commands[i].transform;
    }
    return result;
}

} // namespace Lgt::System
