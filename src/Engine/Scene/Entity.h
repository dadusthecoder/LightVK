#pragma once
#include "Engine/Scene/World.h"
#include "Engine/Core/Logger.h"

namespace Lgt {

class Entity {

public:
    Entity() = default;
    Entity(entt::entity handle, World* world)
        : handle_(handle),
          world_(world) {}

    template <typename T, typename... Args> decltype(auto) Add(Args&&... args);
    template <typename T> decltype(auto)                   Get();
    template <typename T> decltype(auto)                   Get() const;
    template <typename T> void                             Remove();
    template <typename T> bool                             Has() const;

    [[nodiscard]]
    constexpr entt::entity Handle() const noexcept {
        return handle_;
    }

    [[nodiscard]] bool IsValid() const noexcept { return handle_ != entt::null && world_ && world_->Registry().valid(handle_); }

    constexpr bool     operator==(const Entity& o) const noexcept { return handle_ == o.handle_ && world_ == o.world_; }
    constexpr bool     operator!=(const Entity& o) const noexcept { return !(*this == o); }
    constexpr explicit operator bool() const noexcept { return IsValid(); }

    inline static Entity Null() noexcept { return Entity(entt::null, nullptr); }

private:
    inline void  Validate() const;
    entt::entity handle_ = entt::null;
    World*       world_  = nullptr;
};

inline void Entity::Validate() const {
    LGT_ASSERT(handle_ != entt::null, "Null entity");
    LGT_ASSERT(world_ != nullptr, "Null world");
    LGT_ASSERT(world_->Registry().valid(handle_), "Destroyed entity");
}

template <typename T, typename... Args> inline decltype(auto) Entity::Add(Args&&... args) {
    LGT_ASSERT_STATIC(!std::is_reference_v<T>, "Component type cannot be a reference");
    if (!LIGHTVK_VERIFY(!Has<T>(), "Component already exists on this entity! Use Get()")) {}
    return world_->Registry().emplace<T>(handle_, std::forward<Args>(args)...);
}

template <typename T> inline void Entity::Remove() {
    LGT_ASSERT_STATIC(!std::is_reference_v<T>, "Component type cannot be a reference");
    if (!LIGHTVK_VERIFY(Has<T>(), "Component dose not exists on this entity! Use Add()")) {}
    world_->Registry().remove<T>(handle_);
}

template <typename T> inline decltype(auto) Entity::Get() {
    LGT_ASSERT_STATIC(!std::is_reference_v<T>, "Component type cannot be a reference");
    if (!LIGHTVK_VERIFY(Has<T>(), "Component dose not exists on this entity! Use Add()")) {}
    return world_->Registry().get<T>(handle_);
}

template <typename T> inline decltype(auto) Entity::Get() const {
    LGT_ASSERT_STATIC(!std::is_reference_v<T>, "Component type cannot be a reference");
    if (!LIGHTVK_VERIFY(Has<T>(), "Component dose not exists on this entity! Use Add()")) {}
    return world_->Registry().get<T>(handle_);
}

template <typename T> inline bool Entity::Has() const {
    LGT_ASSERT_STATIC(!std::is_reference_v<T>, "Component type cannot be a reference");
    Validate();
    return world_->Registry().all_of<T>(handle_);
}
//-------------------------------------------------------------------------------

} // namespace Lgt
