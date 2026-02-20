---@meta

--- 3D vector type for positions, directions, and math operations.
---@class Vec3
---@field x number X component
---@field y number Y component
---@field z number Z component
---@operator add(Vec3): Vec3
---@operator sub(Vec3): Vec3
---@operator mul(number): Vec3
local Vec3 = {}

--- Create a new Vec3.
---@overload fun(): Vec3
---@overload fun(scalar: number): Vec3
---@param x number
---@param y number
---@param z number
---@return Vec3
function Vec3(x, y, z) end

--- Get the length (magnitude) of this vector.
---@return number
function Vec3:Length() end

--- Get a normalized copy of this vector (unit length).
---@return Vec3
function Vec3:Normalized() end

--- Compute the dot product with another vector.
---@param other Vec3
---@return number
function Vec3:Dot(other) end

--- Compute the cross product with another vector.
---@param other Vec3
---@return Vec3
function Vec3:Cross(other) end
