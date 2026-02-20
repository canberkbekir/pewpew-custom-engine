---@meta

--- The current scene. Provides entity management, physics forces, and raycasting.
--- Accessed via `self._scene` in scripts.
---@class Scene
local Scene = {}

-- =====================
-- Entity Management
-- =====================

--- Find the first entity with the given name.
---@param name string
---@return Entity
function Scene:FindEntity(name) end

--- Create a new entity with the given name.
---@param name string
---@return Entity
function Scene:CreateEntity(name) end

--- Destroy an entity and remove it from the scene.
---@param entity Entity
function Scene:DestroyEntity(entity) end

--- Get an entity by its UUID.
---@param uuid integer
---@return Entity
function Scene:GetEntityByUUID(uuid) end

--- Check if an entity with the given UUID exists.
---@param uuid integer
---@return boolean
function Scene:EntityExists(uuid) end

--- Get the entity with the primary (active) camera, or nil if none.
---@return Entity|nil
function Scene:GetMainCamera() end

--- Get the GameManager entity (the entity with a GameManagerComponent).
---@return Entity|nil
function Scene:GetGameManager() end

-- =====================
-- Physics Forces
-- =====================

--- Apply a continuous force to an entity's rigid body (in Newtons).
--- The force is applied for one physics step.
---@param entity Entity
---@param force Vec3
function Scene:AddForce(entity, force) end

--- Apply a torque to an entity's rigid body.
---@param entity Entity
---@param torque Vec3
function Scene:AddTorque(entity, torque) end

--- Apply an instantaneous impulse to an entity's rigid body.
---@param entity Entity
---@param impulse Vec3
function Scene:AddImpulse(entity, impulse) end

--- Get the linear velocity of an entity's rigid body.
---@param entity Entity
---@return Vec3
function Scene:GetLinearVelocity(entity) end

--- Set the linear velocity of an entity's rigid body.
---@param entity Entity
---@param velocity Vec3
function Scene:SetLinearVelocity(entity, velocity) end

--- Get the angular velocity of an entity's rigid body.
---@param entity Entity
---@return Vec3
function Scene:GetAngularVelocity(entity) end

--- Set the angular velocity of an entity's rigid body.
---@param entity Entity
---@param velocity Vec3
function Scene:SetAngularVelocity(entity, velocity) end

-- =====================
-- Raycasting
-- =====================

--- Cast a ray and return the first hit, or nil if nothing was hit.
---@param origin Vec3 Ray start position
---@param direction Vec3 Ray direction (does not need to be normalized)
---@param maxDistance? number Maximum ray distance (default: 1000)
---@param layerMask? integer Bitmask of layers to test against (default: all layers)
---@param ignoreEntity? Entity Entity to ignore during the raycast
---@return RaycastHit|nil
function Scene:Raycast(origin, direction, maxDistance, layerMask, ignoreEntity) end

--- Cast a ray and return all hits as a table.
---@param origin Vec3 Ray start position
---@param direction Vec3 Ray direction
---@param maxDistance? number Maximum ray distance (default: 1000)
---@param layerMask? integer Bitmask of layers to test against (default: all layers)
---@param ignoreEntity? Entity Entity to ignore during the raycast
---@return RaycastHit[]
function Scene:RaycastAll(origin, direction, maxDistance, layerMask, ignoreEntity) end
