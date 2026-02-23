---@meta

--- Physics query API. All functions require the scene as the first argument.
--- Access the scene from a script via `self._scene`.
---@class Physic
Physic = {}

--- Cast a ray and return the first hit, or nil if nothing was hit.
---@param scene Scene
---@param origin Vector3 Ray start position
---@param direction Vector3 Ray direction (does not need to be normalized)
---@param maxDistance? number Maximum ray distance (default: 1000)
---@param layerMask? integer Bitmask of layers to test against (default: all layers)
---@param ignoreEntity? Entity Entity to ignore during the raycast
---@return RaycastHit|nil
function Physic.Raycast(scene, origin, direction, maxDistance, layerMask, ignoreEntity) end

--- Cast a ray and return all hits as a table.
---@param scene Scene
---@param origin Vector3 Ray start position
---@param direction Vector3 Ray direction
---@param maxDistance? number Maximum ray distance (default: 1000)
---@param layerMask? integer Bitmask of layers to test against (default: all layers)
---@param ignoreEntity? Entity Entity to ignore during the raycast
---@return RaycastHit[]
function Physic.RaycastAll(scene, origin, direction, maxDistance, layerMask, ignoreEntity) end

--- Return all entities whose physics shape overlaps a sphere.
---@param scene Scene
---@param center Vector3 Sphere center in world space
---@param radius number Sphere radius
---@param layerMask? integer Bitmask of layers to test (default: all layers)
---@param ignoreEntity? Entity Entity to exclude
---@return RaycastHit[]
function Physic.OverlapSphere(scene, center, radius, layerMask, ignoreEntity) end

--- Return all entities whose physics shape overlaps an axis-aligned box.
---@param scene Scene
---@param center Vector3 Box center in world space
---@param halfExtents Vector3 Box half-extents on each axis
---@param layerMask? integer Bitmask of layers to test (default: all layers)
---@param ignoreEntity? Entity Entity to exclude
---@return RaycastHit[]
function Physic.OverlapBox(scene, center, halfExtents, layerMask, ignoreEntity) end
