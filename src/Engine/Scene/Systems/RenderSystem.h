#pragma once
#include "Engine/Renderer/Gpu/Renderer.h"

namespace Lgt {
class World;
namespace Assets { class AssetManager; }
}

namespace Lgt::System {

class Render {
public:
    explicit Render(World* world, Assets::AssetManager* assets);

    /// Ensures all MeshFilter assets are loaded on GPU.
    void SyncAssets();

    /// Builds the complete scene draw list for this frame.
    Gpu::DrawList BuildDrawList() const;

private:
    World* _world;
    Assets::AssetManager* _assets;
};

} // namespace Lgt::System
