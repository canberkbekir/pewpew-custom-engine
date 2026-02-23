---@meta

--- The current scene. Provides entity management and scene queries.
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
-- Component Queries
-- =====================

--- Find the first entity that has a given component.
--- Pass a component token (Transform, RigidBody, Collider, Camera, MeshRenderer,
--- VoxelRenderer, DirectionalLight, AudioSource, Script).
---@param componentToken table
---@return Entity
function Scene:FindFirstWithComponent(componentToken) end

--- Find all entities that have a given component.
--- Pass a component token (Transform, RigidBody, Collider, Camera, MeshRenderer,
--- VoxelRenderer, DirectionalLight, AudioSource, Script).
---@param componentToken table
---@return Entity[]
function Scene:FindAllWithComponent(componentToken) end
