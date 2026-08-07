#pragma once
#include "Editor/Context.h"
#include <ImNodeFlow.h>

namespace Lgt {
namespace Editor::Panel {

class NodeEditorPanel {
public:
    void Init(Context* context);
    void Shutdown();
    void Draw();

private:
    Context* _context = nullptr;
    std::unique_ptr<ImFlow::ImNodeFlow> _nodeEditor;
};

} // namespace Editor::Panel
} // namespace Lgt
