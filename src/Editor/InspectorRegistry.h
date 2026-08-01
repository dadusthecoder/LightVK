#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "Engine/Scene/Entity.h"

namespace Lgt::Editor {

using ComponentDrawCallback = std::function<void(Entity, void* componentData)>;

struct ComponentInspectorData {
    std::string name;
    ComponentDrawCallback drawCallback;
    size_t componentSize;
};

class InspectorRegistry {
public:
    template<typename T>
    static void Register(const std::string& name, ComponentDrawCallback callback) {
        GetRegistry()[entt::type_id<T>().hash()] = { name, callback, sizeof(T) };
    }

    static const std::unordered_map<entt::id_type, ComponentInspectorData>& GetInspectors() {
        return GetRegistry();
    }

private:
    static std::unordered_map<entt::id_type, ComponentInspectorData>& GetRegistry() {
        static std::unordered_map<entt::id_type, ComponentInspectorData> registry;
        return registry;
    }
};

} // namespace Lgt::Editor

#define REGISTER_INSPECTOR(ComponentType, DrawFunc) \
    namespace { \
        static bool _reg_##DrawFunc = (Lgt::Editor::InspectorRegistry::Register<ComponentType>(#ComponentType, DrawFunc), true); \
    }
