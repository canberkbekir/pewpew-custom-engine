---@meta

--- Proxy object for a ColliderComponent. Obtained via entity:GetComponent(Collider).
---@class Collider
local Collider = {}

--- Get the collider shape as a string: "box", "sphere", "capsule", "voxel_compound", or "none".
---@return string
function Collider:GetShape() end

--- Check if the collider is a trigger (non-physical).
---@return boolean
function Collider:IsTrigger() end

--- Get the collider offset from the entity origin.
---@return Vec3
function Collider:GetOffset() end

--- Set the collider offset from the entity origin.
---@param offset Vec3
function Collider:SetOffset(offset) end

--- Get the half-extents (for box colliders).
---@return Vec3
function Collider:GetHalfExtents() end

--- Get the radius (for sphere colliders).
---@return number
function Collider:GetRadius() end

--- Check if the collider proxy is valid.
---@return boolean
function Collider:IsValid() end
