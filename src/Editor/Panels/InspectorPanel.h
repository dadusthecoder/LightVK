#pragma once
#include "Editor/Context.h"

namespace Lgt::Editor::Panel {

class InspectorPanel {
public:
    void Init(Context* context);
    void Shutdown();
    void Draw();

private:
    void DrawEntityHeader(Entity entity);
    void DrawLiveStatusBar(Entity entity);
    void DrawComponentTimeline(Entity entity);
    void DrawComponents(Entity entity);
    void DrawAddComponentMenu(Entity entity);

    Context* _context = nullptr;
};

} // namespace Lgt::Editor::Panel
