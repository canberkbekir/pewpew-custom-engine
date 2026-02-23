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

---Returns euler angles (in radians) as a Vector3.
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

---Instantiate a blueprint into the scene. Returns a table of created entities (root first).
---Accepts a BlueprintHandle (from a BlueprintRef field set to a .blueprint asset)
---or an Entity (from a BlueprintRef set to a scene entity, or passed directly).
---When passed an Entity, its entire hierarchy is cloned with new UUIDs.
---@param source BlueprintHandle|Entity
---@return Entity[]
function Scene:InstantiateBlueprint(source) end

---@class BlueprintHandle
---Unified spawnable handle from a BlueprintRef field.
---Carries either a .blueprint asset UUID or a scene entity UUID.
---Pass directly to Scene:InstantiateBlueprint — no branching needed in script.
local BlueprintHandle = {}

---Returns true if the handle has any valid source (asset or entity).
---@return boolean
function BlueprintHandle:IsValid() end

---Returns true if the source is a .blueprint asset file.
---@return boolean
function BlueprintHandle:IsAsset() end

---Returns true if the source is a scene entity (hierarchy clone).
---@return boolean
function BlueprintHandle:IsEntity() end

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
---Accepts drag-drop of a .blueprint file from the content browser.
---At runtime the field receives the blueprint file path as a string.
---Pass the path to Scene:InstantiateBlueprint() to spawn it.
---@return FieldDef
function BlueprintRef() end

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
