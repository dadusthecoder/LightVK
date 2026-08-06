#pragma once

namespace Lgt {
class Entity;
class SceneGraph {
public:
    SceneGraph() = default;
    void Detach(Entity entity);
    void Attach(Entity child, Entity parent);
    void SetParent(Entity parent, Entity child);
    void Reparent(Entity child, Entity newParent);
    void Unparent(Entity child);

private:
    [[nodiscard]] bool IsDescendant(Entity parent, Entity entity) const noexcept;
};
} // namespace Lgt
