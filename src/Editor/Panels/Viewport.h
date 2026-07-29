#pragma once
#include "Editor/Context.h"

namespace Lgt {
namespace Editor::Panel {

class Viewport {
public:
    void Init(Context* context);
    void Shutdown();
    void Draw();

private:
    Context* context_ = nullptr;
};

} // namespace Editor::Panel
} // namespace Lgt
