#include "Editor/Context.h"

namespace Lgt {

namespace Editor::Panel {

class Hierarchy {
public:
    void Init(Context* context);
    void Shutdown();
    void Draw();

private:
    void     DrawNode(Entity entity);
    Context* _context = nullptr;
};
} // namespace Editor::Panel

} // namespace Lgt
