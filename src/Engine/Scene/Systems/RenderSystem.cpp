#include "RenderSystem.h"
#include "Engine/Scene/World.h"
#include "Engine/Scene/Components.h"
#include "Engine/Assets/AssetManager.h"

namespace Lgt::System {

Render::Render(World* world, Assets::AssetManager* assets)
    : _world(world), _assets(assets) {}

void Render::SyncAssets() {
    //_assets->SyncSceneAssets(_world);
}

Gpu::DrawList Render::BuildDrawList() const {
    Gpu::DrawList result;
    auto view = _world->Registry().view<Component::ModelInstance, Component::WorldTransform>();
    
    for (auto entity : view) {
        const auto& instance = view.get<Component::ModelInstance>(entity);
        const auto& worldTransform = view.get<Component::WorldTransform>(entity);

        if (!instance.visible) continue;

        const auto* gpuModel = _assets->GetGpuModel(instance.model);
        if (!gpuModel) continue;

        const size_t firstCommand = result.commands.size();
        result.commands.insert(result.commands.end(),
            gpuModel->drawList.commands.begin(), gpuModel->drawList.commands.end());
        result.indexCounts.insert(result.indexCounts.end(),
            gpuModel->drawList.indexCounts.begin(), gpuModel->drawList.indexCounts.end());

        for (size_t i = firstCommand; i < result.commands.size(); ++i)
            result.commands[i].transform = worldTransform.matrix * result.commands[i].transform;
    }
    return result;
}

} // namespace Lgt::System
