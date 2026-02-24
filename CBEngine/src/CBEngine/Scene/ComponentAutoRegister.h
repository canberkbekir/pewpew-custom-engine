#pragma once

// ============================================================================
// ComponentAutoRegister — zero-touch component serialization registration
//
// Usage (in ComponentRegistrations.cpp — NEVER in headers):
//
//   CB_REGISTER_COMPONENT(MyComponent)
//
// That single line registers the component into RuntimeComponentRegistry so
// it is automatically serialized, deserialized, and asset-resolved everywhere.
// Adding a new component to the engine only requires:
//   1. The component struct has  Serialize(YAML::Emitter&)  and
//      Deserialize(const YAML::Node&)  methods plus a YAMLKey constexpr.
//   2. One CB_REGISTER_COMPONENT line in ComponentRegistrations.cpp.
//   Nothing else.
// ============================================================================

#include "RuntimeComponentRegistry.h"
#include "CBEngine/Utils/YAMLHelpers.h"
#include <yaml-cpp/yaml.h>
#include <type_traits>

namespace CB
{
    // SFINAE: does T have a ResolveAssets() method?
    template<typename T, typename = void>
    struct ComponentHasResolveAssets : std::false_type {};

    template<typename T>
    struct ComponentHasResolveAssets<T, std::void_t<decltype(std::declval<T&>().ResolveAssets())>>
        : std::true_type {};

    // -----------------------------------------------------------------------
    // ComponentAutoRegister<T>
    // Instantiating one of these (via the macro below) registers T into
    // RuntimeComponentRegistry at static-init time, before main() runs.
    // -----------------------------------------------------------------------
    template<typename T>
    struct ComponentAutoRegister
    {
        ComponentAutoRegister()
        {
            RuntimeComponentRegistry::Register(
                std::string(T::YAMLKey),

                // --- Serialize ---
                [](YAML::Emitter& out, entt::registry& reg, entt::entity e)
                {
                    if (!reg.all_of<T>(e))
                        return;
                    out << YAML::Key << T::YAMLKey << YAML::BeginMap;
                    reg.get<T>(e).Serialize(out);
                    out << YAML::EndMap;
                },

                // --- Deserialize ---
                // Uses existing component if already present (Tag/Transform are
                // pre-added by CreateEntityWithUUID), otherwise emplaces a new one.
                [](const YAML::Node& node, entt::registry& reg, entt::entity e)
                {
                    T& comp = reg.all_of<T>(e) ? reg.get<T>(e) : reg.emplace<T>(e);
                    comp.Deserialize(node);
                },

                // --- ResolveAssets (only wired up if T has the method) ---
                [](entt::registry& reg, entt::entity e)
                {
                    if constexpr (ComponentHasResolveAssets<T>::value)
                    {
                        if (reg.all_of<T>(e))
                            reg.get<T>(e).ResolveAssets();
                    }
                }
            );
        }
    };

} // namespace CB

// ---------------------------------------------------------------------------
// CB_REGISTER_COMPONENT(T)
//
// Place this macro once per component in ComponentRegistrations.cpp.
// It creates a file-scope static that calls Register() before main().
// Using a plain static (not inline) is intentional — this is a .cpp file so
// there is exactly one definition and no ODR concern.
// ---------------------------------------------------------------------------
#define CB_REGISTER_COMPONENT(T) \
    static ::CB::ComponentAutoRegister<T> _s_auto_reg_##T {};
