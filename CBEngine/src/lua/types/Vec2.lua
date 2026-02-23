---@meta

--- 2D vector type. Used for mouse delta, scroll delta, and 2D math.
---@class Vector2
---@field x number X component
---@field y number Y component
---@operator add(Vector2): Vector2
---@operator sub(Vector2): Vector2
---@operator mul(number): Vector2
local Vector2 = {}

--- Create a new Vector2.
---@overload fun(): Vector2
---@overload fun(scalar: number): Vector2
---@param x number
---@param y number
---@return Vector2
function Vector2(x, y) end

--- Get the length (magnitude) of this vector.
---@return number
function Vector2:Length() end

--- Get a normalized copy of this vector (unit length).
---@return Vector2
function Vector2:Normalized() end

--- Compute the dot product with another vector.
---@param other Vector2
---@return number
function Vector2:Dot(other) end

--- Linearly interpolate towards another vector.
---@param other Vector2
---@param t number Interpolation factor (0=self, 1=other)
---@return Vector2
function Vector2:Lerp(other, t) end

--- Get the distance to another vector.
---@param other Vector2
---@return number
function Vector2:Distance(other) end
