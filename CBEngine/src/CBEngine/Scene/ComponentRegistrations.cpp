#include "cbpch.h"
#include "ComponentAutoRegister.h"

// Pull in all component headers via the central umbrella
#include "CBEngine/Components/Components.h"
#include "CBEngine/Components/DirectionalLightComponent.h"
#include "CBEngine/Components/AudioSourceComponent.h"

// ============================================================================
// All YAML-serializable components are registered here.
//
// When you create a new component:
//   1. Add   CB_REGISTER_COMPONENT(YourComponent)   below.
//   2. That's it — save, serialize, snapshot/restore all work automatically.
//
// Do NOT register BlueprintInstanceComponent here — it is handled manually
// in SceneSerializer to prevent it from leaking into .blueprint files.
// ============================================================================

namespace CB
{
    CB_REGISTER_COMPONENT(TagComponent)
    CB_REGISTER_COMPONENT(TransformComponent)
    CB_REGISTER_COMPONENT(MeshRendererComponent)
    CB_REGISTER_COMPONENT(VoxelRendererComponent)
    CB_REGISTER_COMPONENT(DirectionalLightComponent)
    CB_REGISTER_COMPONENT(CameraComponent)
    CB_REGISTER_COMPONENT(RigidBodyComponent)
    CB_REGISTER_COMPONENT(ColliderComponent)
    CB_REGISTER_COMPONENT(ScriptComponent)
    CB_REGISTER_COMPONENT(GameManagerComponent)
    CB_REGISTER_COMPONENT(AudioSourceComponent)
    CB_REGISTER_COMPONENT(HingeJointComponent)
    CB_REGISTER_COMPONENT(HingeChainComponent)
    CB_REGISTER_COMPONENT(DestructibleVoxelComponent)
    CB_REGISTER_COMPONENT(VoxelAnchorComponent)

    // Called from Application::Application() to force the linker to include
    // this object file from the static library (MSVC dead-strips objects that
    // export no referenced symbols, which would leave the registry empty).
    void LinkComponentRegistrations() {}
}
