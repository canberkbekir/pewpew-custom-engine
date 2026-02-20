---@meta

--- Result of a raycast query.
---@class RaycastHit
---@field point Vec3 The world-space point where the ray hit
---@field normal Vec3 The surface normal at the hit point
---@field fraction number Hit fraction along the ray (0.0 to 1.0)
---@field distance number Distance from the ray origin to the hit point
local RaycastHit = {}

--- Get the Entity that was hit.
---@param scene Scene The scene to look up the entity in
---@return Entity
function RaycastHit:GetEntity(scene) end
