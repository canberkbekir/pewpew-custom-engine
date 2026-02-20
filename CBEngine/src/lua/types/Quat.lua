---@meta

--- Quaternion type for rotations.
---@class Quat
---@field w number W component (scalar part)
---@field x number X component
---@field y number Y component
---@field z number Z component
local Quat = {}

--- Create a new quaternion.
---@overload fun(): Quat
---@param w number
---@param x number
---@param y number
---@param z number
---@return Quat
function Quat(w, x, y, z) end

--- Convert this quaternion to euler angles (in radians).
---@return Vec3
function Quat:EulerAngles() end
