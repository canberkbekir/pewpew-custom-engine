---@meta

--- An entity in the scene. Provides access to transform, physics, hierarchy, and components.
---@class Entity
local Entity = {}

-- =====================
-- Transform
-- =====================

--- Get the local position of this entity.
---@return Vec3
function Entity:GetPosition() end

--- Set the local position of this entity.
---@param position Vec3
function Entity:SetPosition(position) end

--- Get the local position of this entity (alias for GetPosition).
---@return Vec3
function Entity:GetLocalPosition() end

--- Set the local position of this entity (alias for SetPosition).
---@param position Vec3
function Entity:SetLocalPosition(position) end

--- Get the world-space position of this entity (accounts for parent hierarchy).
---@return Vec3
function Entity:GetWorldPosition() end

--- Get the local rotation as euler angles (in degrees).
---@return Vec3
function Entity:GetRotation() end

--- Set the local rotation from euler angles (in degrees).
---@param rotation Vec3
function Entity:SetRotation(rotation) end

--- Get the local rotation as euler angles (in degrees). Alias for GetRotation.
---@return Vec3
function Entity:GetLocalRotation() end

--- Set the local rotation from euler angles (in degrees). Alias for SetRotation.
---@param rotation Vec3
function Entity:SetLocalRotation(rotation) end

--- Get the local scale of this entity.
---@return Vec3
function Entity:GetScale() end

--- Set the local scale of this entity.
---@param scale Vec3
function Entity:SetScale(scale) end

--- Get the local scale of this entity (alias for GetScale).
---@return Vec3
function Entity:GetLocalScale() end

--- Set the local scale of this entity (alias for SetScale).
---@param scale Vec3
function Entity:SetLocalScale(scale) end

--- Get the forward direction vector (local Z axis in world space).
---@return Vec3
function Entity:GetForward() end

--- Get the right direction vector (local X axis in world space).
---@return Vec3
function Entity:GetRight() end

--- Get the up direction vector (local Y axis in world space).
---@return Vec3
function Entity:GetUp() end

--- Get the Transform component proxy for this entity.
--- Always valid (all entities have a Transform).
---@return Transform
function Entity:GetTransform() end

--- Move the entity by a delta in local space. Shorthand for SetPosition(GetPosition() + delta).
---@param delta Vec3
function Entity:Translate(delta) end

--- Rotate the entity by euler angle deltas (in degrees). Shorthand for SetRotation(GetRotation() + delta).
---@param delta Vec3
function Entity:Rotate(delta) end

--- Get the world-space rotation as euler angles (in degrees), derived from the world matrix.
---@return Vec3
function Entity:GetWorldRotation() end

--- Orient the entity to look towards a world-space target point.
--- Sets the local rotation using a look-at quaternion.
---@param target Vec3 World-space target position
---@param up? Vec3 Up vector (default: Vec3(0, 1, 0))
function Entity:LookAt(target, up) end

-- =====================
-- Identity
-- =====================

--- Get the display name (TagComponent) of this entity.
---@return string
function Entity:GetName() end

--- Set the display name (TagComponent) of this entity.
---@param name string
function Entity:SetName(name) end

--- Get the unique identifier (UUID) of this entity.
---@return integer
function Entity:GetUUID() end

--- Check if this entity handle is valid (points to a live entity).
---@return boolean
function Entity:IsValid() end

-- =====================
-- Hierarchy
-- =====================

--- Set the parent of this entity.
---@param parent Entity The new parent entity
---@param keepWorldTransform? boolean Whether to preserve world position (default: true)
function Entity:SetParent(parent, keepWorldTransform) end

--- Remove the parent of this entity (make it a root entity).
---@param keepWorldTransform? boolean Whether to preserve world position (default: true)
function Entity:RemoveParent(keepWorldTransform) end

--- Get the parent entity, or an invalid entity if this is a root.
---@return Entity
function Entity:GetParent() end

--- Get a list of all direct child entities.
---@return Entity[]
function Entity:GetChildren() end

--- Check if this entity has a parent.
---@return boolean
function Entity:HasParent() end

--- Check if this entity has any children.
---@return boolean
function Entity:HasChildren() end

--- Check if this entity is a descendant of the given ancestor.
---@param ancestor Entity
---@return boolean
function Entity:IsDescendantOf(ancestor) end

-- =====================
-- Visibility
-- =====================

--- Set the visibility of this entity's renderer (MeshRenderer or VoxelRenderer).
---@param visible boolean
function Entity:SetVisible(visible) end

--- Check if this entity's renderer is visible.
---@return boolean
function Entity:IsVisible() end

-- =====================
-- Component Checks
-- =====================

--- Check if this entity has a RigidBodyComponent.
---@return boolean
function Entity:HasRigidBody() end

--- Check if this entity has a ColliderComponent.
---@return boolean
function Entity:HasCollider() end

--- Check if this entity has a MeshRendererComponent.
---@return boolean
function Entity:HasMeshRenderer() end

--- Check if this entity has a VoxelRendererComponent.
---@return boolean
function Entity:HasVoxelRenderer() end

--- Check if this entity has a DirectionalLightComponent.
---@return boolean
function Entity:HasDirectionalLight() end

--- Check if this entity has a ScriptComponent.
---@return boolean
function Entity:HasScript() end

--- Check if this entity has a CameraComponent.
---@return boolean
function Entity:HasCamera() end

-- =====================
-- Camera
-- =====================

--- Get the camera field of view (degrees).
---@return number
function Entity:GetFOV() end

--- Set the camera field of view (degrees).
---@param fov number
function Entity:SetFOV(fov) end

--- Get the camera near clip distance.
---@return number
function Entity:GetNearClip() end

--- Set the camera near clip distance.
---@param nearClip number
function Entity:SetNearClip(nearClip) end

--- Get the camera far clip distance.
---@return number
function Entity:GetFarClip() end

--- Set the camera far clip distance.
---@param farClip number
function Entity:SetFarClip(farClip) end

--- Check if this entity's camera is the primary (active) camera.
---@return boolean
function Entity:IsPrimaryCamera() end

--- Set whether this entity's camera is the primary (active) camera.
---@param primary boolean
function Entity:SetPrimaryCamera(primary) end

-- =====================
-- Physics Properties
-- =====================

--- Get the mass of the rigid body.
---@return number
function Entity:GetMass() end

--- Set the mass of the rigid body.
---@param mass number
function Entity:SetMass(mass) end

--- Get the friction coefficient of the rigid body.
---@return number
function Entity:GetFriction() end

--- Set the friction coefficient of the rigid body.
---@param friction number
function Entity:SetFriction(friction) end

--- Get the restitution (bounciness) of the rigid body.
---@return number
function Entity:GetRestitution() end

--- Set the restitution (bounciness) of the rigid body.
---@param restitution number
function Entity:SetRestitution(restitution) end

--- Get the linear damping of the rigid body.
---@return number
function Entity:GetLinearDamping() end

--- Set the linear damping of the rigid body.
---@param damping number
function Entity:SetLinearDamping(damping) end

--- Get the angular damping of the rigid body.
---@return number
function Entity:GetAngularDamping() end

--- Set the angular damping of the rigid body.
---@param damping number
function Entity:SetAngularDamping(damping) end

--- Check if the rigid body uses gravity.
---@return boolean
function Entity:IsUsingGravity() end

--- Set whether the rigid body uses gravity.
---@param useGravity boolean
function Entity:SetUseGravity(useGravity) end

--- Get the body type as a string: "static", "dynamic", or "kinematic".
---@return string
function Entity:GetBodyType() end

--- Set the body type. Use BodyType.Static, BodyType.Dynamic, or BodyType.Kinematic.
---@param bodyType string
function Entity:SetBodyType(bodyType) end

-- =====================
-- Collider
-- =====================

--- Get the collider shape as a string: "box", "sphere", "capsule", "voxel_compound", or "none".
---@return string
function Entity:GetColliderShape() end

--- Check if the collider is a trigger (non-physical).
---@return boolean
function Entity:IsTrigger() end

-- =====================
-- Layer
-- =====================

--- Get the physics/rendering layer index of this entity.
---@return integer
function Entity:GetLayer() end

--- Set the physics/rendering layer index of this entity.
---@param layer integer
function Entity:SetLayer(layer) end

-- =====================
-- Scripting
-- =====================

--- Get a script instance attached to this entity by its class table.
--- Returns nil if the script is not found.
---@param classTable table The class table (e.g., `PlayerController`)
---@return table|nil
function Entity:GetScript(classTable) end

--- Get a built-in component proxy or script instance by token/class table.
--- For built-in components, pass the global token: RigidBody, Transform, Camera, Collider, MeshRenderer, VoxelRenderer, DirectionalLight.
--- For scripts, pass the class table (e.g. PlayerMovement).
--- Also accepts legacy string names (e.g. "RigidBody").
---@param token table|string Component token table or class table
---@return RigidBody|Transform|Camera|Collider|MeshRenderer|VoxelRenderer|DirectionalLight|table|nil
function Entity:GetComponent(token) end
