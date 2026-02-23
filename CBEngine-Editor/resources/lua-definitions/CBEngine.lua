--- CBEngine Lua API Definitions
--- EmmyLua annotations for VS Code / Lua Language Server IntelliSense.
--- This file is NOT executed — it only provides type information.
---@meta

--------------------------------------------------------------------------------
-- Vector2
--------------------------------------------------------------------------------

---@class Vector2
---@field x number
---@field y number
---@operator add(Vector2): Vector2
---@operator sub(Vector2): Vector2
---@operator mul(number): Vector2
local Vector2 = {}

---@overload fun(): Vector2
---@overload fun(scalar: number): Vector2
---@overload fun(x: number, y: number): Vector2
---@return Vector2
function Vector2(...) end

---@return number
function Vector2:Length() end
---@return Vector2
function Vector2:Normalized() end
---@param other Vector2
---@return number
function Vector2:Dot(other) end
---@param other Vector2
---@param t number
---@return Vector2
function Vector2:Lerp(other, t) end
---@param other Vector2
---@return number
function Vector2:Distance(other) end

--------------------------------------------------------------------------------
-- Vector3
--------------------------------------------------------------------------------

---@class Vector3
---@field x number
---@field y number
---@field z number
---@operator add(Vector3): Vector3
---@operator sub(Vector3): Vector3
---@operator mul(number): Vector3
local Vector3 = {}

---@overload fun(): Vector3
---@overload fun(scalar: number): Vector3
---@overload fun(x: number, y: number, z: number): Vector3
---@return Vector3
function Vector3(...) end

---Returns the length (magnitude) of this vector.
---@return number
function Vector3:Length() end

---Returns a normalized copy of this vector.
---@return Vector3
function Vector3:Normalized() end

---Returns the dot product of two vectors.
---@param other Vector3
---@return number
function Vector3:Dot(other) end

---Returns the cross product of two vectors.
---@param other Vector3
---@return Vector3
function Vector3:Cross(other) end

---Linearly interpolates towards another vector.
---@param other Vector3
---@param t number Interpolation factor (0=self, 1=other)
---@return Vector3
function Vector3:Lerp(other, t) end

---Returns the distance to another vector.
---@param other Vector3
---@return number
function Vector3:Distance(other) end

-- Static direction constants
---@type Vector3
Vector3.zero    = nil
---@type Vector3
Vector3.one     = nil
---@type Vector3
Vector3.up      = nil
---@type Vector3
Vector3.down    = nil
---@type Vector3
Vector3.forward = nil
---@type Vector3
Vector3.back    = nil
---@type Vector3
Vector3.right   = nil
---@type Vector3
Vector3.left    = nil

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

---Returns euler angles in DEGREES as a Vector3 (pitch=x, yaw=y, roll=z).
---@return Vector3
function Quat:EulerAngles() end

--------------------------------------------------------------------------------
-- Math
--------------------------------------------------------------------------------

---Capitalized math API. Use instead of the built-in `math` table for consistency.
---@class CBEngine.MathAPI
Math = {}

--- Pi constant (~3.14159).
---@type number
Math.Pi = 3.14159265358979323846

-- Rounding / integer
---@param x number @return number
function Math.Floor(x) end
---@param x number @return number
function Math.Ceil(x) end
---@param x number @return number
function Math.Round(x) end
---@param x number @return number
function Math.Abs(x) end
---Returns 1, -1, or 0 depending on the sign of x.
---@param x number @return number
function Math.Sign(x) end

-- Range / interpolation
---@param x number @param min number @param max number @return number
function Math.Clamp(x, min, max) end
---@param a number @param b number @param t number @return number
function Math.Lerp(a, b, t) end
---@param a number @param b number @return number
function Math.Min(a, b) end
---@param a number @param b number @return number
function Math.Max(a, b) end
---Hermite interpolation (0 when x<=edge0, 1 when x>=edge1).
---@param edge0 number @param edge1 number @param x number @return number
function Math.Smoothstep(edge0, edge1, x) end
---Move `current` towards `target` by at most `maxDelta`, without overshooting.
---@param current number @param target number @param maxDelta number @return number
function Math.MoveTowards(current, target, maxDelta) end
---Ping-pong t between 0 and length.
---@param t number @param length number @return number
function Math.PingPong(t, length) end

-- Power / log
---@param x number @return number
function Math.Sqrt(x) end
---@param x number @param y number @return number
function Math.Pow(x, y) end
---e^x
---@param x number @return number
function Math.Exp(x) end
---Natural logarithm.
---@param x number @return number
function Math.Log(x) end

-- Trigonometry
---@param x number Angle in radians @return number
function Math.Sin(x) end
---@param x number Angle in radians @return number
function Math.Cos(x) end
---@param x number Angle in radians @return number
function Math.Tan(x) end
---@param x number @return number
function Math.Asin(x) end
---@param x number @return number
function Math.Acos(x) end
---@param y number @param x number @return number
function Math.Atan2(y, x) end
---Convert degrees to radians.
---@param deg number @return number
function Math.Rad(deg) end
---Convert radians to degrees.
---@param rad number @return number
function Math.Deg(rad) end

--------------------------------------------------------------------------------
-- Time
--------------------------------------------------------------------------------

---@class CBEngine.TimeAPI
---@field DeltaTime number @ Seconds since last frame
---@field TotalTime number @ Total seconds elapsed since play mode started
---@field FrameCount integer @ Number of frames rendered since play mode started
Time = {}

--------------------------------------------------------------------------------
-- Entity
--------------------------------------------------------------------------------

---@class CBEngine.Entity
local Entity = {}

-- Transform --

---Get entity local position.
---@return Vector3
function Entity:GetPosition() end

---Set entity local position.
---@param position Vector3
function Entity:SetPosition(position) end

---Get entity local position (alias for GetPosition).
---@return Vector3
function Entity:GetLocalPosition() end

---Set entity local position (alias for SetPosition).
---@param position Vector3
function Entity:SetLocalPosition(position) end

---Get entity world position (takes parent hierarchy into account).
---@return Vector3
function Entity:GetWorldPosition() end

---Get entity local rotation as euler angles (radians).
---@return Vector3
function Entity:GetRotation() end

---Set entity local rotation from euler angles (radians).
---@param rotation Vector3
function Entity:SetRotation(rotation) end

---Get entity local rotation as euler angles (radians). Alias for GetRotation.
---@return Vector3
function Entity:GetLocalRotation() end

---Set entity local rotation from euler angles (radians). Alias for SetRotation.
---@param rotation Vector3
function Entity:SetLocalRotation(rotation) end

---Get entity local scale.
---@return Vector3
function Entity:GetScale() end

---Set entity local scale.
---@param scale Vector3
function Entity:SetScale(scale) end

---Get entity local scale (alias for GetScale).
---@return Vector3
function Entity:GetLocalScale() end

---Set entity local scale (alias for SetScale).
---@param scale Vector3
function Entity:SetLocalScale(scale) end

---Get the entity's local forward direction vector.
---@return Vector3
function Entity:GetForward() end

---Get the entity's local right direction vector.
---@return Vector3
function Entity:GetRight() end

---Get the entity's local up direction vector.
---@return Vector3
function Entity:GetUp() end

---Get the Transform component proxy for this entity.
---Always valid (all entities have a Transform).
---@return Transform
function Entity:GetTransform() end

---Move the entity by a delta. Shorthand for SetPosition(GetPosition() + delta).
---@param delta Vector3
function Entity:Translate(delta) end

---Rotate the entity by euler angle deltas (radians). Shorthand for SetRotation(GetRotation() + delta).
---@param delta Vector3
function Entity:Rotate(delta) end

---Get the world-space rotation as euler angles (degrees), derived from the world matrix.
---@return Vector3
function Entity:GetWorldRotation() end

---Orient this entity to look towards a world-space target point.
---@param target Vector3 World-space target position
---@param up? Vector3 Up vector (default: Vector3(0,1,0))
function Entity:LookAt(target, up) end


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
---@return boolean
function Entity:HasAudioSource() end

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

-- Layer --

---Get the physics/rendering layer index.
---@return integer
function Entity:GetLayer() end

---Set the physics/rendering layer index.
---@param layer integer
function Entity:SetLayer(layer) end

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

---Get the entity with the primary camera.
---@return Entity|nil
function Scene:GetMainCamera() end

---Get the GameManager entity.
---@return Entity|nil
function Scene:GetGameManager() end

---Find the first entity that has the given component.
---Pass a component token (Transform, RigidBody, Collider, Camera, MeshRenderer,
---VoxelRenderer, DirectionalLight, AudioSource, Script).
---@param componentToken table
---@return Entity
function Scene:FindFirstWithComponent(componentToken) end

---Find all entities that have the given component.
---Pass a component token (Transform, RigidBody, Collider, Camera, MeshRenderer,
---VoxelRenderer, DirectionalLight, AudioSource, Script).
---@param componentToken table
---@return Entity[]
function Scene:FindAllWithComponent(componentToken) end

---Instantiate a blueprint or clone an entity into the scene.
---Returns a table of created entities (root first, then children in hierarchy order).
---
---Overload 1: pass a BlueprintHandle (from a BlueprintRef field pointing to a .blueprint asset).
---Overload 2: pass an Entity (from a BlueprintRef field pointing to a scene entity,
---            or any entity handle) — clones the entity and its entire child hierarchy.
---@overload fun(self: Scene, handle: BlueprintHandle): Entity[]
---@overload fun(self: Scene, template: Entity): Entity[]
---@return Entity[]
function Scene:Instantiate(source) end

---@class BlueprintHandle
---Handle for a .blueprint asset assigned via a BlueprintRef field.
---Pass to Scene:Instantiate(handle) to spawn the blueprint with new UUIDs.
local BlueprintHandle = {}

---Returns true if this handle holds a valid asset UUID.
---@return boolean
function BlueprintHandle:IsValid() end

--------------------------------------------------------------------------------
-- Physic
--------------------------------------------------------------------------------

---Physics query API. All functions take the scene as first argument.
---Access via `self._scene` in script callbacks.
---@class CBEngine.PhysicAPI
Physic = {}

---Cast a ray and return the first hit, or nil if nothing was hit.
---@param scene Scene
---@param origin Vector3 Ray start position
---@param direction Vector3 Ray direction
---@param maxDistance? number Default: 1000
---@param layerMask? integer Default: Layer.All
---@param ignoreEntity? Entity Entity to exclude
---@return RaycastHit|nil
function Physic.Raycast(scene, origin, direction, maxDistance, layerMask, ignoreEntity) end

---Cast a ray and return all hits as a table.
---@param scene Scene
---@param origin Vector3
---@param direction Vector3
---@param maxDistance? number Default: 1000
---@param layerMask? integer Default: Layer.All
---@param ignoreEntity? Entity
---@return RaycastHit[]
function Physic.RaycastAll(scene, origin, direction, maxDistance, layerMask, ignoreEntity) end

---Return all entities whose physics shape overlaps a sphere.
---@param scene Scene
---@param center Vector3 Sphere center in world space
---@param radius number
---@param layerMask? integer Default: Layer.All
---@param ignoreEntity? Entity
---@return RaycastHit[]
function Physic.OverlapSphere(scene, center, radius, layerMask, ignoreEntity) end

---Return all entities whose physics shape overlaps an axis-aligned box.
---@param scene Scene
---@param center Vector3 Box center in world space
---@param halfExtents Vector3 Box half-extents on each axis
---@param layerMask? integer Default: Layer.All
---@param ignoreEntity? Entity
---@return RaycastHit[]
function Physic.OverlapBox(scene, center, halfExtents, layerMask, ignoreEntity) end

--------------------------------------------------------------------------------
-- Input
--------------------------------------------------------------------------------

---@class CBEngine.InputAPI
Input = {}

---Check if a keyboard or mouse key is currently held.
---@param keyCode integer @ Use Key.* constants (e.g., Key.W, Key.Space, Key.MouseLeft)
---@return boolean
function Input.IsKeyPressed(keyCode) end

---Check if a mouse button is currently pressed.
---@param button integer @ Use Mouse.* constants (e.g., Mouse.Left)
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

---Check if a key was pressed this frame only (one-shot, does not repeat).
---@param keyCode integer @ Use Key.* constants
---@return boolean
function Input.IsKeyJustPressed(keyCode) end

---Check if a key was released this frame only (one-shot).
---@param keyCode integer @ Use Key.* constants
---@return boolean
function Input.IsKeyJustReleased(keyCode) end

---Get raw mouse movement since the last frame as a Vector2.
---When cursor is locked, bypasses OS pointer acceleration.
---@return Vector2
function Input.GetMouseDelta() end

---Get mouse scroll wheel delta this frame as a Vector2.
---x = horizontal scroll, y = vertical scroll.
---@return Vector2
function Input.GetMouseScrollDelta() end

---Lock and hide the cursor for FPS look. Also enables raw mouse motion.
---@param locked boolean
function Input.SetCursorLocked(locked) end

---Returns true if the cursor is currently locked.
---@return boolean
function Input.IsCursorLocked() end

--------------------------------------------------------------------------------
-- Key Constants
--------------------------------------------------------------------------------

---@class CBEngine.KeyConstants
Key = {
    -- Printable
    Space = 32, Apostrophe = 39, Comma = 44, Minus = 45, Period = 46,
    Slash = 47, Semicolon = 59, Equal = 61,
    LeftBracket = 91, Backslash = 92, RightBracket = 93, GraveAccent = 96,
    -- Digits
    Num0 = 48, Num1 = 49, Num2 = 50, Num3 = 51, Num4 = 52,
    Num5 = 53, Num6 = 54, Num7 = 55, Num8 = 56, Num9 = 57,
    -- Letters
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72,
    I = 73, J = 74, K = 75, L = 76, M = 77, N = 78, O = 79, P = 80,
    Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
    Y = 89, Z = 90,
    -- Control
    Escape = 256, Enter = 257, Tab = 258, Backspace = 259,
    Insert = 260, Delete = 261, Right = 262, Left = 263, Down = 264, Up = 265,
    PageUp = 266, PageDown = 267, Home = 268, End = 269,
    CapsLock = 280, ScrollLock = 281, NumLock = 282, PrintScreen = 283, Pause = 284,
    -- Function keys
    F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294, F6 = 295,
    F7 = 296, F8 = 297, F9 = 298, F10 = 299, F11 = 300, F12 = 301,
    -- Modifiers
    LeftShift = 340, LeftControl = 341, LeftAlt = 342, LeftSuper = 343,
    RightShift = 344, RightControl = 345, RightAlt = 346, RightSuper = 347,
    -- Numpad
    KP0 = 320, KP1 = 321, KP2 = 322, KP3 = 323, KP4 = 324,
    KP5 = 325, KP6 = 326, KP7 = 327, KP8 = 328, KP9 = 329,
    KPDecimal = 330, KPDivide = 331, KPMultiply = 332,
    KPSubtract = 333, KPAdd = 334, KPEnter = 335, KPEqual = 336,
    -- Mouse buttons (unified — use with IsKeyPressed / IsKeyJustPressed)
    MouseLeft = 1000, MouseRight = 1001, MouseMiddle = 1002,
    MouseX1 = 1003, MouseX2 = 1004,
    MouseButton6 = 1005, MouseButton7 = 1006, MouseButton8 = 1007,
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
-- Layer Constants
--------------------------------------------------------------------------------

---@class CBEngine.LayerConstants
---@field Default integer
---@field Player integer
---@field Enemy integer
---@field Environment integer
---@field Projectile integer
---@field Trigger integer
---@field IgnoreRaycast integer
---@field All integer
Layer = {}

---Build a layer bitmask from multiple layer indices.
---@vararg integer Layer indices
---@return integer
function Layer.Mask(...) end

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
-- Debug
--------------------------------------------------------------------------------

---@class CBEngine.DebugAPI
Debug = {}

---Draw a line in world space for one frame (or `duration` seconds if provided).
---@param from Vector3
---@param to Vector3
---@param color? Vector3 @ RGB (default: green)
---@param duration? number @ Seconds (default: 0 = one frame)
function Debug.DrawLine(from, to, color, duration) end

---Draw a ray from an origin in world space.
---@param origin Vector3
---@param direction Vector3
---@param maxDist? number @ Default: 100
---@param color? Vector3 @ RGB (default: red)
---@param duration? number @ Seconds (default: 0 = one frame)
function Debug.DrawRay(origin, direction, maxDist, color, duration) end

--------------------------------------------------------------------------------
-- RaycastHit
--------------------------------------------------------------------------------

---@class RaycastHit
---@field point Vector3 @ World-space hit position
---@field normal Vector3 @ Surface normal at hit point
---@field fraction number @ Fraction along the ray (0=origin, 1=maxDist)
---@field distance number @ World-space distance from ray origin to hit
local RaycastHit = {}

---Get the entity that was hit.
---@param scene Scene
---@return Entity
function RaycastHit:GetEntity(scene) end

--------------------------------------------------------------------------------
-- Field Helper Functions
-- Use ONLY inside __fields = { } blocks to declare inspector fields.
--------------------------------------------------------------------------------

---@class FieldDef
---@field type string
---@field default any
---@field min? number
---@field max? number

---Create a float field.
---@param default? number
---@param min? number
---@param max? number
---@return FieldDef
function Float(default, min, max) end

---Create an int field.
---@param default? integer
---@param min? integer
---@param max? integer
---@return FieldDef
function Int(default, min, max) end

---Create a bool field.
---@param default? boolean
---@return FieldDef
function Bool(default) end

---Create a string field.
---@param default? string
---@return FieldDef
function String(default) end

---Create a color field (RGBA).
---@param r? number @ Red (0-1), default 1
---@param g? number @ Green (0-1), default 1
---@param b? number @ Blue (0-1), default 1
---@param a? number @ Alpha (0-1), default 1
---@return FieldDef
function Color(r, g, b, a) end

---@class Field
Field = {}

---Create a float field.
---@param default? number
---@param min? number
---@param max? number
---@return FieldDef
function Field.Float(default, min, max) end

---Create an int field.
---@param default? integer
---@param min? integer
---@param max? integer
---@return FieldDef
function Field.Int(default, min, max) end

---Create a bool field.
---@param default? boolean
---@return FieldDef
function Field.Bool(default) end

---Create a string field.
---@param default? string
---@return FieldDef
function Field.String(default) end

---Create a color field (RGBA).
---@param r? number @ Red (0-1), default 1
---@param g? number @ Green (0-1), default 1
---@param b? number @ Blue (0-1), default 1
---@param a? number @ Alpha (0-1), default 1
---@return FieldDef
function Field.Color(r, g, b, a) end

---Create a Vector3 field. Use Field.Vector3 ONLY inside __fields = { }.
---For actual vectors in script logic, use Vector3(x, y, z).
---@param x? number @ Default 0
---@param y? number @ Default 0
---@param z? number @ Default 0
---@return FieldDef
function Field.Vector3(x, y, z) end

---Create an entity reference field.
---Accepts drag-drop from the hierarchy. At runtime receives the referenced Entity.
---@return FieldDef
function Field.EntityRef() end

---Create a component reference field.
---@param componentToken table A global component token (RigidBody, Transform, Camera, etc.)
---@return FieldDef
function Field.ComponentRef(componentToken) end

---Create a script reference field.
---@param classTable table The global class table of the target script
---@return FieldDef
function Field.ScriptRef(classTable) end

---Create a blueprint reference field.
---Accepts drag-drop from the content browser (.blueprint assets) OR from the hierarchy (scene entities).
---At runtime: .blueprint drops inject a BlueprintHandle; scene entity drops inject an Entity.
---Both types are accepted by Scene:Instantiate() — no branching needed in script.
---@return FieldDef
function Field.BlueprintRef() end

---Create an audio clip reference field.
---Accepts drag-drop from the content browser (.sfx assets).
---@return FieldDef
function Field.AudioRef() end

---Create a Vector3 field. Use Vec3 ONLY inside __fields = { }.
---For actual vectors in script logic, use Vector3(x, y, z).
---@param x? number @ Default 0
---@param y? number @ Default 0
---@param z? number @ Default 0
---@return FieldDef
function Vec3(x, y, z) end

---Create an entity reference field.
---Accepts drag-drop from the hierarchy. At runtime receives the referenced Entity.
---@return FieldDef
function EntityRef() end

---Create a component reference field.
---@param componentToken table A global component token (RigidBody, Transform, Camera, etc.)
---@return FieldDef
function ComponentRef(componentToken) end

---Create a script reference field.
---@param classTable table The global class table of the target script
---@return FieldDef
function ScriptRef(classTable) end

---Create a blueprint reference field.
---Accepts drag-drop from the content browser (.blueprint assets) OR from the hierarchy (scene entities).
---At runtime: .blueprint drops inject a BlueprintHandle; scene entity drops inject an Entity.
---Both types are accepted by Scene:Instantiate() — no branching needed in script.
---@return FieldDef
function BlueprintRef() end

---Declare a .sfx audio clip reference field in `__fields`.
---At runtime: the editor-assigned .sfx asset is injected as an AudioHandle.
---Pass the handle to Audio.Play() or Audio.PlayAt().
---@return FieldDef
function AudioRef() end

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

---Called after physics update, every frame (ideal for camera follow scripts).
---@param self ScriptSelf
---@param dt number @ Delta time in seconds
function OnLateUpdate(self, dt) end

---Called at a fixed rate (1/60 s) in sync with the physics step.
---Use for physics forces and movement that must be framerate-independent.
---@param self ScriptSelf
---@param fixedDt number @ Fixed delta time (always 1/60)
function OnFixedUpdate(self, fixedDt) end

---Called when the entity is destroyed / play mode stops.
---@param self ScriptSelf
function OnDestroy(self) end

---Called when this entity's collider begins overlapping another (physical contact).
---@param self ScriptSelf
---@param other Entity @ The other entity in the collision
---@param contactPoint Vector3 @ World-space contact point
---@param contactNormal Vector3 @ Contact normal pointing from other towards self
function OnCollisionBegin(self, other, contactPoint, contactNormal) end

---Called when a physical collision with another entity ends.
---@param self ScriptSelf
---@param other Entity @ The other entity
function OnCollisionEnd(self, other) end

---Called when this entity's trigger collider is entered by another entity.
---@param self ScriptSelf
---@param other Entity @ The entity that entered the trigger
---@param contactPoint Vector3 @ World-space entry contact point
---@param contactNormal Vector3 @ Contact normal
function OnTriggerEnter(self, other, contactPoint, contactNormal) end

---Called when an entity exits this entity's trigger collider.
---@param self ScriptSelf
---@param other Entity @ The entity that exited the trigger
function OnTriggerExit(self, other) end

---Called in the editor when any field value changes.
---@param self ScriptSelf
---@param fields table @ Table of current field values
---@param changedField string @ Name of the field that changed
---@return table|nil @ Return corrected fields table or nil
function OnValidate(self, fields, changedField) end

--------------------------------------------------------------------------------
-- Timer
--------------------------------------------------------------------------------

---@class Timer
---@field _duration number
---@field _time number
---@field _loop boolean
---@field _done boolean
---@field _active boolean
Timer = {}

---Create a new timer that fires `callback` after `duration` seconds.
---@param duration number Seconds until fire
---@param callback fun() Function to call on expiry
---@param loop? boolean Restart automatically after firing. Default: false.
---@return Timer
function Timer.New(duration, callback, loop) end

---Advance the timer. Call this in OnUpdate or OnFixedUpdate.
---@param dt number Delta time in seconds
function Timer:Update(dt) end

---Reset elapsed time to zero and reactivate the timer.
function Timer:Reset() end

---Pause the timer without resetting elapsed time.
function Timer:Stop() end

---Resume a stopped timer.
function Timer:Start() end

---Returns true if the timer has fired and is not looping.
---@return boolean
function Timer:IsDone() end

---Returns true if the timer is currently running.
---@return boolean
function Timer:IsActive() end

---Returns elapsed time in seconds.
---@return number
function Timer:GetTime() end

---Returns the timer's total duration in seconds.
---@return number
function Timer:GetDuration() end

---Returns elapsed/duration clamped to [0, 1].
---@return number
function Timer:GetProgress() end

--------------------------------------------------------------------------------
-- Quat additions (Phase 2)
--------------------------------------------------------------------------------

---Returns euler angles in DEGREES as a Vector3. Alias for EulerAngles().
---@return Vector3
function Quat:ToEuler() end

---Rotate a Vector3 by this quaternion.
---@param v Vector3
---@return Vector3
function Quat:Rotate(v) end

---Create a Quat from euler angles in degrees.
---@param euler Vector3 Euler angles in degrees (pitch=x, yaw=y, roll=z)
---@return Quat
function Quat.FromEuler(euler) end

---Create a Quat that looks in the given direction.
---@param forward Vector3 Normalized forward direction
---@param up? Vector3 Up direction (default: Vector3(0,1,0))
---@return Quat
function Quat.LookRotation(forward, up) end

---Spherically interpolate between two quaternions.
---@param a Quat
---@param b Quat
---@param t number Blend factor [0,1]
---@return Quat
function Quat.Slerp(a, b, t) end

---Angle in degrees between two quaternions.
---@param a Quat
---@param b Quat
---@return number
function Quat.AngleBetween(a, b) end

---Identity quaternion (no rotation).
---@type Quat
Quat.Identity = nil

--------------------------------------------------------------------------------
-- Entity additions (Phase 2)
--------------------------------------------------------------------------------

---Clone this entity into the given scene.
---@param scene Scene
---@return Entity
function Entity:Clone(scene) end

--------------------------------------------------------------------------------
-- Scene additions (Phase 2)
--------------------------------------------------------------------------------

---Find all entities with the given name tag.
---@param name string
---@return Entity[]
function Scene:FindAllWithName(name) end

---Return all entities in the scene.
---@return Entity[]
function Scene:GetAllEntities() end

---Return the number of entities in the scene.
---@return integer
function Scene:GetEntityCount() end

---Find all entities whose physics body overlaps a sphere of radius.
---@param center Vector3
---@param radius number
---@param layerMask? integer Default: Layer.All
---@return Entity[]
function Scene:FindEntitiesInRadius(center, radius, layerMask) end

---Deferred scene load (executed at end of current frame).
---@param path string Path to .cbscene file
function Scene:LoadScene(path) end

---Reload the currently active scene.
function Scene:ReloadScene() end

---Get the active scene name.
---@return string
function Scene:GetSceneName() end

---Get the active scene file path.
---@return string
function Scene:GetScenePath() end

---Convert a world-space position to viewport pixel coordinates.
---Returns nil if the point is behind the camera.
---@param worldPos Vector3
---@return Vector2|nil
function Scene:WorldToScreen(worldPos) end

---Return a world-space ray from viewport pixel coordinates.
---@param screenX number Pixel X coordinate
---@param screenY number Pixel Y coordinate
---@return {origin: Vector3, direction: Vector3}
function Scene:ScreenToWorldRay(screenX, screenY) end

--------------------------------------------------------------------------------
-- MaterialProxy (Phase 2)
--------------------------------------------------------------------------------

---@class MaterialProxy
---@field IsValid fun(self: MaterialProxy): boolean
local MaterialProxy = {}

---@return Vector3 Albedo RGB in [0,1]
function MaterialProxy:GetAlbedo() end
---@param v Vector3
function MaterialProxy:SetAlbedo(v) end
---@return number
function MaterialProxy:GetMetallic() end
---@param v number
function MaterialProxy:SetMetallic(v) end
---@return number
function MaterialProxy:GetRoughness() end
---@param v number
function MaterialProxy:SetRoughness(v) end
---@return number
function MaterialProxy:GetSmoothShading() end
---@param v number
function MaterialProxy:SetSmoothShading(v) end

--------------------------------------------------------------------------------
-- CameraProxy additions (Phase 4)
--------------------------------------------------------------------------------

---Convert a world-space position to viewport pixel coordinates.
---Returns nil if the point is behind the camera.
---@param worldPos Vector3
---@return Vector2|nil
function CameraProxy:WorldToScreen(worldPos) end

---Return a world-space ray from viewport pixel coordinates.
---@param screenX number
---@param screenY number
---@return {origin: Vector3, direction: Vector3}
function CameraProxy:ScreenToWorldRay(screenX, screenY) end

--------------------------------------------------------------------------------
-- Physics sweep casts (Phase 3)
--------------------------------------------------------------------------------

---Sphere-cast along a ray, returning the first hit or nil.
---@param scene Scene
---@param origin Vector3
---@param direction Vector3
---@param radius number Sphere radius
---@param maxDistance? number Default: 100
---@param layerMask? integer Default: Layer.All
---@param ignoreEntity? Entity
---@return RaycastHit|nil
function Physic.SphereCast(scene, origin, direction, radius, maxDistance, layerMask, ignoreEntity) end

---Box-cast along a ray, returning the first hit or nil.
---@param scene Scene
---@param origin Vector3
---@param direction Vector3
---@param halfExtents Vector3
---@param maxDistance? number Default: 100
---@param layerMask? integer Default: Layer.All
---@param ignoreEntity? Entity
---@return RaycastHit|nil
function Physic.BoxCast(scene, origin, direction, halfExtents, maxDistance, layerMask, ignoreEntity) end

---Sphere-cast along a ray, returning all hits as a table.
---@param scene Scene
---@param origin Vector3
---@param direction Vector3
---@param radius number
---@param maxDistance? number
---@param layerMask? integer
---@param ignoreEntity? Entity
---@return RaycastHit[]
function Physic.SphereCastAll(scene, origin, direction, radius, maxDistance, layerMask, ignoreEntity) end

---Box-cast along a ray, returning all hits as a table.
---@param scene Scene
---@param origin Vector3
---@param direction Vector3
---@param halfExtents Vector3
---@param maxDistance? number
---@param layerMask? integer
---@param ignoreEntity? Entity
---@return RaycastHit[]
function Physic.BoxCastAll(scene, origin, direction, halfExtents, maxDistance, layerMask, ignoreEntity) end

---Return all entities whose physics shape overlaps a capsule (Y-axis aligned).
---@param scene Scene
---@param center Vector3 Capsule center in world space
---@param radius number Capsule radius
---@param halfHeight number Half-height of the cylindrical portion
---@param layerMask? integer Default: Layer.All
---@param ignoreEntity? Entity
---@return RaycastHit[]
function Physic.OverlapCapsule(scene, center, radius, halfHeight, layerMask, ignoreEntity) end

--------------------------------------------------------------------------------
-- Debug additions (Phase 2)
--------------------------------------------------------------------------------

---Draw a wireframe sphere (3 great-circle arcs) for one or more frames.
---@param center Vector3
---@param radius number
---@param color? Vector3 RGB color (default green)
---@param duration? number Seconds to display (0 = one frame)
function Debug.DrawSphere(center, radius, color, duration) end

---Draw a wireframe axis-aligned box for one or more frames.
---@param center Vector3
---@param halfExtents Vector3
---@param color? Vector3 RGB color
---@param duration? number
function Debug.DrawBox(center, halfExtents, color, duration) end

---Draw a wireframe AABB from min/max corners.
---@param min Vector3
---@param max Vector3
---@param color? Vector3 RGB color
---@param duration? number
function Debug.DrawWireCube(min, max, color, duration) end

---Draw a 3-axis cross marker at a position.
---@param pos Vector3
---@param size number Arm length
---@param color? Vector3 RGB color
---@param duration? number
function Debug.DrawCross(pos, size, color, duration) end

--------------------------------------------------------------------------------
-- Signal (Phase 1)
--------------------------------------------------------------------------------

---@class Connection
local Connection = {}

---Disconnect this listener from its signal.
function Connection:Disconnect() end

---Whether this connection is still active.
---@type boolean
Connection.Connected = true

---@class Signal
local Signal = {}

---Create a new Signal dispatcher.
---@return Signal
function Signal.New() end

---Connect a listener function to this signal.
---Returns a Connection that can be used to unsubscribe.
---@param fn fun(...) Callback function
---@return Connection
function Signal:Connect(fn) end

---Fire the signal, calling all connected listeners with the given args.
function Signal:Fire(...) end

---Disconnect all listeners.
function Signal:DisconnectAll() end

--------------------------------------------------------------------------------
-- Coroutine scheduler (Phase 1)
--------------------------------------------------------------------------------

---@class CBEngine.CoroutineAPI
Coroutine = {}

---Start a coroutine from `fn`. Returns immediately; fn runs on the next tick.
---@param fn fun()
function Coroutine.Start(fn) end

---Yield the current coroutine for `seconds` real-time seconds.
---Only valid inside a Coroutine.Start callback.
---@param seconds number
function Coroutine.Wait(seconds) end

---Yield the current coroutine for `n` frames.
---Only valid inside a Coroutine.Start callback.
---@param n integer
function Coroutine.WaitFrames(n) end

--------------------------------------------------------------------------------
-- Easing / Tween
--------------------------------------------------------------------------------

---@class CBEngine.EasingAPI
Easing = {}
-- Classic (backward compat)
Easing.Linear    = nil  ---@type fun(t:number):number
Easing.EaseIn    = nil  ---@type fun(t:number):number  Quad ease-in
Easing.EaseOut   = nil  ---@type fun(t:number):number  Quad ease-out
Easing.EaseInOut = nil  ---@type fun(t:number):number  Quad ease-in-out
-- Quad
Easing.QuadIn    = nil  ---@type fun(t:number):number
Easing.QuadOut   = nil  ---@type fun(t:number):number
Easing.QuadInOut = nil  ---@type fun(t:number):number
-- Cubic
Easing.CubicIn    = nil ---@type fun(t:number):number
Easing.CubicOut   = nil ---@type fun(t:number):number
Easing.CubicInOut = nil ---@type fun(t:number):number
-- Sine
Easing.SineIn    = nil  ---@type fun(t:number):number
Easing.SineOut   = nil  ---@type fun(t:number):number
Easing.SineInOut = nil  ---@type fun(t:number):number
-- Expo
Easing.ExpoIn    = nil  ---@type fun(t:number):number
Easing.ExpoOut   = nil  ---@type fun(t:number):number
Easing.ExpoInOut = nil  ---@type fun(t:number):number
-- Circ
Easing.CircIn    = nil  ---@type fun(t:number):number
Easing.CircOut   = nil  ---@type fun(t:number):number
Easing.CircInOut = nil  ---@type fun(t:number):number
-- Back (slight overshoot)
Easing.BackIn    = nil  ---@type fun(t:number):number
Easing.BackOut   = nil  ---@type fun(t:number):number
Easing.BackInOut = nil  ---@type fun(t:number):number
-- Bounce
Easing.Bounce      = nil ---@type fun(t:number):number  Alias for BounceOut
Easing.BounceIn    = nil ---@type fun(t:number):number
Easing.BounceOut   = nil ---@type fun(t:number):number
Easing.BounceInOut = nil ---@type fun(t:number):number
-- Elastic
Easing.Elastic      = nil ---@type fun(t:number):number  Alias for ElasticOut
Easing.ElasticIn    = nil ---@type fun(t:number):number
Easing.ElasticOut   = nil ---@type fun(t:number):number
Easing.ElasticInOut = nil ---@type fun(t:number):number

---@class TweenHandle
---Returned by Tween.Value and entity DO* helpers. Supports fluent method chaining.
local TweenHandle = {}

---Set the easing function. Chain-friendly.
---@param fn fun(t:number):number  Use an Easing constant
---@return TweenHandle
function TweenHandle:SetEase(fn) end

---Called each frame with the current interpolated value.
---@param fn fun(value:any)
---@return TweenHandle
function TweenHandle:OnUpdate(fn) end

---Called when the tween completes (after all loops).
---@param fn fun()
---@return TweenHandle
function TweenHandle:OnComplete(fn) end

---Called the first time the tween begins playing (after any delay).
---@param fn fun()
---@return TweenHandle
function TweenHandle:OnStart(fn) end

---Delay before the tween starts, in seconds.
---@param secs number
---@return TweenHandle
function TweenHandle:SetDelay(secs) end

---Loop the tween. `count=-1` loops forever. `loopType` is "Restart" (default) or "Yoyo".
---@param count integer  Number of extra loops (-1 = infinite)
---@param loopType? string  "Restart" | "Yoyo"
---@return TweenHandle
function TweenHandle:SetLoops(count, loopType) end

---Make `to` relative: the tween adds `to` to the starting value instead of going to an absolute target.
---@return TweenHandle
function TweenHandle:SetRelative() end

---Cancel and remove the tween immediately.
function TweenHandle:Cancel() end

---Pause the tween (time stops advancing).
function TweenHandle:Pause() end

---Resume a paused tween.
function TweenHandle:Resume() end

---@return boolean
function TweenHandle:IsActive() end

---@class SequenceHandle
---Returned by Tween.Sequence(). Plays tweens in order or simultaneously.
local SequenceHandle = {}

---Append a tween to play after all previous steps finish.
---@param tween TweenHandle
---@return SequenceHandle
function SequenceHandle:Append(tween) end

---Add a tween to play simultaneously with the last Appended step.
---@param tween TweenHandle
---@return SequenceHandle
function SequenceHandle:Join(tween) end

---Insert a pause gap of `secs` seconds between steps.
---@param secs number
---@return SequenceHandle
function SequenceHandle:AppendInterval(secs) end

---@param secs number
---@return SequenceHandle
function SequenceHandle:SetDelay(secs) end

---@param count integer  -1 = infinite
---@param loopType? string  "Restart" | "Yoyo"
---@return SequenceHandle
function SequenceHandle:SetLoops(count, loopType) end

---@param fn fun()
---@return SequenceHandle
function SequenceHandle:OnComplete(fn) end

---@param fn fun()
---@return SequenceHandle
function SequenceHandle:OnStart(fn) end

function SequenceHandle:Cancel() end
function SequenceHandle:Pause() end
function SequenceHandle:Resume() end
---@return boolean
function SequenceHandle:IsActive() end

---@class CBEngine.TweenAPI
Tween = {}

---Create a tween. Supports fluent chaining.
---
---**Fluent style:**
---```lua
---Tween.Value(0, 10, 1.5)
---    :SetEase(Easing.ExpoOut)
---    :OnUpdate(function(v) entity:SetPosition(Vector3(v, 0, 0)) end)
---    :OnComplete(function() Log.Info("done") end)
---    :SetLoops(-1, "Yoyo")
---```
---**Legacy style (still supported):**
---```lua
---Tween.Value(0, 1, 1.0, Easing.Linear, function(v) ... end, function() ... end)
---```
---@param from number|userdata  Starting value (number, Vector3, or Vector2)
---@param to   number|userdata  Target value
---@param duration number
---@param easing?   fun(t:number):number  Easing function (legacy arg)
---@param onUpdate? fun(value:any)        Update callback (legacy arg)
---@param onComplete? fun()               Complete callback (legacy arg)
---@return TweenHandle
function Tween.Value(from, to, duration, easing, onUpdate, onComplete) end

---Cancel a specific tween immediately.
---@param tween TweenHandle
function Tween.Kill(tween) end

---Cancel all active tweens and sequences.
function Tween.KillAll() end

---Create an ordered / simultaneous sequence of tweens.
---```lua
---local e = self._entity
---local seq = Tween.Sequence()
---seq:Append(Tween.Value(e:GetPosition(), posA, 0.5):OnUpdate(function(v) if e:IsValid() then e:SetPosition(v) end end))
---seq:AppendInterval(0.2)
---seq:Append(Tween.Value(e:GetPosition(), posB, 0.5):OnUpdate(function(v) if e:IsValid() then e:SetPosition(v) end end))
---seq:OnComplete(function() Log.Info("sequence done") end)
---```
---@return SequenceHandle
function Tween.Sequence() end

--------------------------------------------------------------------------------
-- Object Pool (Phase 1)
--------------------------------------------------------------------------------

---@class Pool
local Pool = {}

---Create a new object pool backed by a blueprint asset.
---@param scene Scene
---@param blueprint BlueprintHandle
---@param initialSize? integer
---@return Pool
function Pool.New(scene, blueprint, initialSize) end

---Get a pooled entity (or instantiate a new one if the pool is empty).
---@return Entity
function Pool:Get() end

---Return an entity to the pool (sets it inactive).
---@param entity Entity
function Pool:Return(entity) end

---Pre-instantiate `count` entities into the pool.
---@param count integer
function Pool:Prewarm(count) end

--------------------------------------------------------------------------------
-- AudioHandle (Phase 5)
--------------------------------------------------------------------------------

---@class AudioHandle
---@field AssetUUID integer
local AudioHandle = {}

---@return boolean
function AudioHandle:IsValid() end

--------------------------------------------------------------------------------
-- AudioSourceProxy (Phase 5)
--------------------------------------------------------------------------------

---@class AudioSourceProxy
local AudioSourceProxy = {}

---@return boolean
function AudioSourceProxy:IsValid() end
function AudioSourceProxy:Play() end
function AudioSourceProxy:Stop() end
function AudioSourceProxy:Pause() end
function AudioSourceProxy:Resume() end
---@param v number Volume [0,1]
function AudioSourceProxy:SetVolume(v) end
---@param v number Pitch multiplier
function AudioSourceProxy:SetPitch(v) end
---@return boolean
function AudioSourceProxy:IsPlaying() end

--------------------------------------------------------------------------------
-- Audio API (Phase 5)
--------------------------------------------------------------------------------

---@class CBEngine.AudioAPI
Audio = {}

---Play an audio clip from an AudioHandle field. Returns a source handle.
---@param handle AudioHandle
---@param volumeOverride? number Override volume (< 0 uses clip default)
---@return integer Source handle for control calls
function Audio.Play(handle, volumeOverride) end

---Play an audio clip at a 3D world position.
---@param handle AudioHandle
---@param position Vector3
---@param volumeOverride? number
---@return integer Source handle
function Audio.PlayAt(handle, position, volumeOverride) end

---Stop a playing source.
---@param sourceHandle integer
function Audio.Stop(sourceHandle) end

---Stop all playing sources.
function Audio.StopAll() end

---Pause a playing source.
---@param sourceHandle integer
function Audio.Pause(sourceHandle) end

---Resume a paused source.
---@param sourceHandle integer
function Audio.Resume(sourceHandle) end

---Set volume on a specific source.
---@param sourceHandle integer
---@param volume number [0,1]
function Audio.SetVolume(sourceHandle, volume) end

---Set master volume for all audio.
---@param volume number [0,1]
function Audio.SetMasterVolume(volume) end

---Check if a source is still playing.
---@param sourceHandle integer
---@return boolean
function Audio.IsPlaying(sourceHandle) end

--------------------------------------------------------------------------------
-- SceneManager (Phase 6)
--------------------------------------------------------------------------------

---@class CBEngine.SceneManagerAPI
SceneManager = {}

---Deferred scene load — executes at end of the current frame.
---@param path string Path to .cbscene file (relative to working directory)
function SceneManager.LoadScene(path) end

---Reload the currently active scene.
function SceneManager.ReloadScene() end

---Get the name of the currently active scene.
---@return string
function SceneManager.GetSceneName() end

---Get the file path of the currently active scene.
---@return string
function SceneManager.GetScenePath() end
