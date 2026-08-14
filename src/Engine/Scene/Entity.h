#pragma once
#include "Engine/Scene/World.h"
#include "Engine/Core/Logger.h"

namespace Lgt {

class Entity {

public:
    Entity() = default;
    Entity(entt::entity handle, World* world)
        : _handle(handle),
          _world(world) {}

    template <typename T, typename... Args> decltype(auto) Add(Args&&... args);
    template <typename T> decltype(auto)                   Get();
    template <typename T> decltype(auto)                   Get() const;
    template <typename T> void                             Remove();
    template <typename T> bool                             Has() const;

    [[nodiscard]]
    constexpr entt::entity Handle() const noexcept {
        return _handle;
    }
    
    [[nodiscard]] World* GetWorld() const noexcept { return _world; }

    [[nodiscard]] bool IsValid() const noexcept { return _handle != entt::null && _world && _world->Registry().valid(_handle); }

    constexpr bool     operator==(const Entity& o) const noexcept { return _handle == o._handle && _world == o._world; }
    constexpr bool     operator!=(const Entity& o) const noexcept { return !(*this == o); }
    constexpr explicit operator bool() const noexcept { return IsValid(); }

    inline static Entity Null() noexcept { return Entity(entt::null, nullptr); }

private:
    inline void  Validate() const;
    entt::entity _handle = entt::null;
    World*       _world  = nullptr;
};

inline void Entity::Validate() const {
    LGT_ASSERT(_handle != entt::null, "Null entity");
    LGT_ASSERT(_world != nullptr, "Null world");
    LGT_ASSERT(_world->Registry().valid(_handle), "Destroyed entity");
}

template <typename T, typename... Args> inline decltype(auto) Entity::Add(Args&&... args) {
    LGT_ASSERT_STATIC(!std::is_reference_v<T>, "Component type cannot be a reference");
    if (!LIGHTVK_VERIFY(!Has<T>(), "Component already exists on this entity! Use Get()")) {}
    return _world->Registry().emplace<T>(_handle, std::forward<Args>(args)...);
}

template <typename T> inline void Entity::Remove() {
    LGT_ASSERT_STATIC(!std::is_reference_v<T>, "Component type cannot be a reference");
    if (!LIGHTVK_VERIFY(Has<T>(), "Component dose not exists on this entity! Use Add()")) {}
    _world->Registry().remove<T>(_handle);
}

template <typename T> inline decltype(auto) Entity::Get() {
    LGT_ASSERT_STATIC(!std::is_reference_v<T>, "Component type cannot be a reference");
    if (!LIGHTVK_VERIFY(Has<T>(), "Component dose not exists on this entity! Use Add()")) {}
    return _world->Registry().get<T>(_handle);
}

template <typename T> inline decltype(auto) Entity::Get() const {
    LGT_ASSERT_STATIC(!std::is_reference_v<T>, "Component type cannot be a reference");
    if (!LIGHTVK_VERIFY(Has<T>(), "Component dose not exists on this entity! Use Add()")) {}
    return _world->Registry().get<T>(_handle);
}

template <typename T> inline bool Entity::Has() const {
    LGT_ASSERT_STATIC(!std::is_reference_v<T>, "Component type cannot be a reference");
    Validate();
    return _world->Registry().all_of<T>(_handle);
}
//-------------------------------------------------------------------------------

} // namespace Lgt
