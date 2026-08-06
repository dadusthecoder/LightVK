#pragma once
#include "Editor/Context.h"

namespace Lgt::Editor::Panel {

class ConsolePanel {
public:
    void Init(Context* context);
    void Shutdown();
    void Draw();

private:
    Context* _context = nullptr;
    bool _autoScroll = true;
    bool _showTrace = true;
    bool _showInfo = true;
    bool _showWarn = true;
    bool _showError = true;
    char _filterText[256] = {};
};

} // namespace Lgt::Editor::Panel
