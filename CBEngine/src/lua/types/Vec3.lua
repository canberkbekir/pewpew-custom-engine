---@meta

--- 3D vector type for positions, directions, and math operations.
---@class Vector3
---@field x number X component
---@field y number Y component
---@field z number Z component
---@operator add(Vector3): Vector3
---@operator sub(Vector3): Vector3
---@operator mul(number): Vector3
local Vector3 = {}

--- Create a new Vector3.
---@overload fun(): Vector3
---@overload fun(scalar: number): Vector3
---@param x number
---@param y number
---@param z number
---@return Vector3
function Vector3(x, y, z) end

--- Get the length (magnitude) of this vector.
---@return number
function Vector3:Length() end

--- Get a normalized copy of this vector (unit length).
---@return Vector3
function Vector3:Normalized() end

--- Compute the dot product with another vector.
---@param other Vector3
---@return number
function Vector3:Dot(other) end

--- Compute the cross product with another vector.
---@param other Vector3
---@return Vector3
function Vector3:Cross(other) end

--- Linearly interpolate towards another vector.
---@param other Vector3
---@param t number Interpolation factor (0=self, 1=other)
---@return Vector3
function Vector3:Lerp(other, t) end

--- Get the distance to another vector.
---@param other Vector3
---@return number
function Vector3:Distance(other) end

-- Static constants
Vector3.zero    = Vector3(0, 0, 0)  ---@type Vector3
Vector3.one     = Vector3(1, 1, 1)  ---@type Vector3
Vector3.up      = Vector3(0, 1, 0)  ---@type Vector3
Vector3.down    = Vector3(0,-1, 0)  ---@type Vector3
Vector3.forward = Vector3(0, 0, 1)  ---@type Vector3
Vector3.back    = Vector3(0, 0,-1)  ---@type Vector3
Vector3.right   = Vector3(1, 0, 0)  ---@type Vector3
Vector3.left    = Vector3(-1,0, 0)  ---@type Vector3
