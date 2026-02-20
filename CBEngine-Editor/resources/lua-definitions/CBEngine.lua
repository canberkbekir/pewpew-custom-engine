--- CBEngine Lua API Definitions
--- Auto-generated EmmyLua annotations for VS Code IntelliSense
--- This file is NOT executed — it only provides type information to the Lua Language Server.
---@meta

--------------------------------------------------------------------------------
-- Vec3
--------------------------------------------------------------------------------

---@class Vec3
---@field x number
---@field y number
---@field z number
---@operator add(Vec3): Vec3
---@operator sub(Vec3): Vec3
---@operator mul(number): Vec3
local Vec3 = {}

---@overload fun(): Vec3
---@overload fun(scalar: number): Vec3
---@overload fun(x: number, y: number, z: number): Vec3
---@return Vec3
function Vec3(...) end

---Returns the length (magnitude) of this vector.
---@return number
function Vec3:Length() end

---Returns a normalized copy of this vector.
---@return Vec3
function Vec3:Normalized() end

---Returns the dot product of two vectors.
---@param other Vec3
---@return number
function Vec3:Dot(other) end

---Returns the cross product of two vectors.
---@param other Vec3
---@return Vec3
function Vec3:Cross(other) end

--------------------------------------------------------------------------------
-- Quat
--------------------------------------------------------------------------------

---@class Quat
---@field w number
---@field x number
---@field y number
---@field z number
local Quat = {}

---@overload fun(): Quat
---@overload fun(w: number, x: number, y: number, z: number): Quat
---@return Quat
function Quat(...) end

---Returns euler angles (in radians) as a Vec3.
---@return Vec3
function Quat:EulerAngles() end

--------------------------------------------------------------------------------
-- Entity
--------------------------------------------------------------------------------

---@class CBEngine.Entity
local Entity = {}

-- Transform --

---Get entity local position.
---@return Vec3
function Entity:GetPosition() end

---Set entity local position.
---@param position Vec3
function Entity:SetPosition(position) end

---Get entity world position (takes parent hierarchy into account).
---@return Vec3
function Entity:GetWorldPosition() end

---Get entity local rotation as euler angles (radians).
---@return Vec3
function Entity:GetRotation() end

---Set entity local rotation from euler angles (radians).
---@param rotation Vec3
function Entity:SetRotation(rotation) end

---Get entity local scale.
---@return Vec3
function Entity:GetScale() end

---Set entity local scale.
---@param scale Vec3
function Entity:SetScale(scale) end

---Get the entity's local forward direction vector.
---@return Vec3
function Entity:GetForward() end

---Get the entity's local right direction vector.
---@return Vec3
function Entity:GetRight() end

---Get the entity's local up direction vector.
---@return Vec3
function Entity:GetUp() end

-- Identity --

---Get the entity's display name.
---@return string
function Entity:GetName() end

---Set the entity's display name.
---@param name string
function Entity:SetName(name) end

---Get the entity's unique ID (uint64 as number).
---@return integer
function Entity:GetUUID() end

---Check if the entity handle is valid.
---@return boolean
function Entity:IsValid() end

-- Hierarchy --

---Set a parent entity. Child transform stays in world space by default.
---@param parent Entity
---@param keepWorldTransform? boolean @ Default: true
function Entity:SetParent(parent, keepWorldTransform) end

---Remove the parent. Child transform stays in world space by default.
---@param keepWorldTransform? boolean @ Default: true
function Entity:RemoveParent(keepWorldTransform) end

---Get the parent entity (invalid Entity if none).
---@return Entity
function Entity:GetParent() end

---Get a table of all child entities.
---@return Entity[]
function Entity:GetChildren() end

---Returns true if this entity has a parent.
---@return boolean
function Entity:HasParent() end

---Returns true if this entity has children.
---@return boolean
function Entity:HasChildren() end

---Returns true if this entity is a descendant of the given ancestor.
---@param ancestor Entity
---@return boolean
function Entity:IsDescendantOf(ancestor) end

-- Visibility --

---Set visibility of the MeshRenderer or VoxelRenderer component.
---@param visible boolean
function Entity:SetVisible(visible) end

---Check visibility of the MeshRenderer or VoxelRenderer component.
---@return boolean
function Entity:IsVisible() end

-- Component Checks --

---@return boolean
function Entity:HasRigidBody() end

---@return boolean
function Entity:HasCollider() end

---@return boolean
function Entity:HasMeshRenderer() end

---@return boolean
function Entity:HasVoxelRenderer() end

---@return boolean
function Entity:HasDirectionalLight() end

---@return boolean
function Entity:HasScript() end

---@return boolean
function Entity:HasCamera() end

-- Camera --

---Get camera field of view in degrees.
---@return number
function Entity:GetFOV() end

---Set camera field of view in degrees.
---@param fov number
function Entity:SetFOV(fov) end

---Get camera near clip plane distance.
---@return number
function Entity:GetNearClip() end

---Set camera near clip plane distance.
---@param nearClip number
function Entity:SetNearClip(nearClip) end

---Get camera far clip plane distance.
---@return number
function Entity:GetFarClip() end

---Set camera far clip plane distance.
---@param farClip number
function Entity:SetFarClip(farClip) end

---Returns true if this is the primary game camera.
---@return boolean
function Entity:IsPrimaryCamera() end

---Set whether this is the primary game camera.
---@param primary boolean
function Entity:SetPrimaryCamera(primary) end

-- Physics: RigidBody Properties --

---@return number
function Entity:GetMass() end

---@param mass number
function Entity:SetMass(mass) end

---@return number
function Entity:GetFriction() end

---@param friction number
function Entity:SetFriction(friction) end

---@return number
function Entity:GetRestitution() end

---@param restitution number
function Entity:SetRestitution(restitution) end

---@return number
function Entity:GetLinearDamping() end

---@param damping number
function Entity:SetLinearDamping(damping) end

---@return number
function Entity:GetAngularDamping() end

---@param damping number
function Entity:SetAngularDamping(damping) end

---@return boolean
function Entity:IsUsingGravity() end

---@param useGravity boolean
function Entity:SetUseGravity(useGravity) end

---Get body type as string: "static", "dynamic", "kinematic", or "none".
---@return string
function Entity:GetBodyType() end

---Set body type. Use BodyType constants or strings: "static", "dynamic", "kinematic".
---@param type string
function Entity:SetBodyType(type) end

-- Physics: Collider Queries --

---Get collider shape as string: "box", "sphere", "capsule", "voxel_compound", or "none".
---@return string
function Entity:GetColliderShape() end

---Returns true if the collider is a trigger (non-physical).
---@return boolean
function Entity:IsTrigger() end

--------------------------------------------------------------------------------
-- Scene
--------------------------------------------------------------------------------

---@class CBEngine.Scene
local Scene = {}

---Find the first entity with the given name. Returns invalid Entity if not found.
---@param name string
---@return Entity
function Scene:FindEntity(name) end

---Create a new entity with the given name.
---@param name string
---@return Entity
function Scene:CreateEntity(name) end

---Destroy an entity.
---@param entity Entity
function Scene:DestroyEntity(entity) end

---Get an entity by its UUID.
---@param uuid integer
---@return Entity
function Scene:GetEntityByUUID(uuid) end

---Check if an entity with the given UUID exists.
---@param uuid integer
---@return boolean
function Scene:EntityExists(uuid) end

-- Physics: Forces & Velocity (require active physics simulation) --

---Apply a force to an entity (requires RigidBody with active body).
---@param entity Entity
---@param force Vec3
function Scene:AddForce(entity, force) end

---Apply a torque to an entity.
---@param entity Entity
---@param torque Vec3
function Scene:AddTorque(entity, torque) end

---Apply an instantaneous impulse to an entity.
---@param entity Entity
---@param impulse Vec3
function Scene:AddImpulse(entity, impulse) end

---Get the linear velocity of an entity.
---@param entity Entity
---@return Vec3
function Scene:GetLinearVelocity(entity) end

---Set the linear velocity of an entity.
---@param entity Entity
---@param velocity Vec3
function Scene:SetLinearVelocity(entity, velocity) end

---Get the angular velocity of an entity.
---@param entity Entity
---@return Vec3
function Scene:GetAngularVelocity(entity) end

---Set the angular velocity of an entity.
---@param entity Entity
---@param velocity Vec3
function Scene:SetAngularVelocity(entity, velocity) end

--------------------------------------------------------------------------------
-- Input
--------------------------------------------------------------------------------

---@class CBEngine.InputAPI
Input = {}

---Check if a key is currently pressed.
---@param keyCode integer @ Use Key.* constants
---@return boolean
function Input.IsKeyPressed(keyCode) end

---Check if a mouse button is currently pressed.
---@param button integer @ Use Mouse.* constants
---@return boolean
function Input.IsMouseButtonPressed(button) end

---Get the current mouse position.
---@return number x, number y
function Input.GetMousePosition() end

---Get the current mouse X position.
---@return number
function Input.GetMouseX() end

---Get the current mouse Y position.
---@return number
function Input.GetMouseY() end

--------------------------------------------------------------------------------
-- Key Constants
--------------------------------------------------------------------------------

---@class CBEngine.KeyConstants
Key = {
    Space = 32,
    Escape = 256,
    Enter = 257,
    Tab = 258,
    Backspace = 259,
    Insert = 260,
    Delete = 261,
    Right = 262,
    Left = 263,
    Down = 264,
    Up = 265,
    PageUp = 266,
    PageDown = 267,
    Home = 268,
    End = 269,
    CapsLock = 280,
    F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294, F6 = 295,
    F7 = 296, F8 = 297, F9 = 298, F10 = 299, F11 = 300, F12 = 301,
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72,
    I = 73, J = 74, K = 75, L = 76, M = 77, N = 78, O = 79, P = 80,
    Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
    Y = 89, Z = 90,
    Num0 = 48, Num1 = 49, Num2 = 50, Num3 = 51, Num4 = 52,
    Num5 = 53, Num6 = 54, Num7 = 55, Num8 = 56, Num9 = 57,
    LeftShift = 340,
    LeftControl = 341,
    LeftAlt = 342,
    RightShift = 344,
    RightControl = 345,
    RightAlt = 346,
}

--------------------------------------------------------------------------------
-- Mouse Constants
--------------------------------------------------------------------------------

---@class CBEngine.MouseConstants
Mouse = {
    Left = 0,
    Right = 1,
    Middle = 2,
}

--------------------------------------------------------------------------------
-- BodyType Constants
--------------------------------------------------------------------------------

---@class CBEngine.BodyTypeConstants
BodyType = {
    Static = "static",
    Dynamic = "dynamic",
    Kinematic = "kinematic",
}

--------------------------------------------------------------------------------
-- Log
--------------------------------------------------------------------------------

---@class CBEngine.LogAPI
Log = {}

---@param message string
function Log.Info(message) end

---@param message string
function Log.Warn(message) end

---@param message string
function Log.Error(message) end

---@param message string
function Log.Trace(message) end

--------------------------------------------------------------------------------
-- Field Helper Functions
--------------------------------------------------------------------------------

---@class FieldDef
---@field type string
---@field default any
---@field min? number
---@field max? number

---Create a float field descriptor.
---@param default? number
---@param min? number
---@param max? number
---@return FieldDef
function Float(default, min, max) end

---Create an int field descriptor.
---@param default? integer
---@param min? integer
---@param max? integer
---@return FieldDef
function Int(default, min, max) end

---Create a bool field descriptor.
---@param default? boolean
---@return FieldDef
function Bool(default) end

---Create a string field descriptor.
---@param default? string
---@return FieldDef
function String(default) end

---Create a color field descriptor (RGBA).
---@param r? number @ Red (0-1), default 1
---@param g? number @ Green (0-1), default 1
---@param b? number @ Blue (0-1), default 1
---@param a? number @ Alpha (0-1), default 1
---@return FieldDef
function Color(r, g, b, a) end

---Create a Vector3 field descriptor.
---@param x? number @ Default 0
---@param y? number @ Default 0
---@param z? number @ Default 0
---@return FieldDef
function Vector3(x, y, z) end

--------------------------------------------------------------------------------
-- Script Lifecycle (override these in your scripts)
--------------------------------------------------------------------------------

---@class ScriptSelf
---@field _entity Entity @ The entity this script is attached to
---@field _scene Scene @ The scene the entity belongs to

---Called once when the entity is created / play mode starts.
---@param self ScriptSelf
function OnCreate(self) end

---Called every frame with the delta time.
---@param self ScriptSelf
---@param dt number @ Delta time in seconds
function OnUpdate(self, dt) end

---Called when the entity is destroyed / play mode stops.
---@param self ScriptSelf
function OnDestroy(self) end

---Called when this entity collides with another entity.
---@param self ScriptSelf
---@param other Entity @ The other entity in the collision
---@param contactPoint Vec3 @ World-space contact point
---@param contactNormal Vec3 @ Contact normal pointing from other towards self
function OnCollisionBegin(self, other, contactPoint, contactNormal) end

---Called when a collision with another entity ends.
---@param self ScriptSelf
---@param other Entity @ The other entity
function OnCollisionEnd(self, other) end

---Called in the editor when any field value changes. Return a corrected fields
---table to override values, or return nothing to accept the change as-is.
---@param self ScriptSelf
---@param fields table @ Table of current field values
---@param changedField string @ Name of the field that changed
---@return table|nil @ Return corrected fields table or nil
function OnValidate(self, fields, changedField) end
