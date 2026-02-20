---@meta

--- Proxy object for a TransformComponent. Obtained via entity:GetComponent(Transform).
---@class Transform
local Transform = {}

--- Get the local position.
---@return Vec3
function Transform:GetPosition() end

--- Set the local position.
---@param position Vec3
function Transform:SetPosition(position) end

--- Get the world-space position (accounts for parent hierarchy).
---@return Vec3
function Transform:GetWorldPosition() end

--- Get the local rotation as euler angles (in radians).
---@return Vec3
function Transform:GetRotation() end

--- Set the local rotation from euler angles (in radians).
---@param rotation Vec3
function Transform:SetRotation(rotation) end

--- Get the local scale.
---@return Vec3
function Transform:GetScale() end

--- Set the local scale.
---@param scale Vec3
function Transform:SetScale(scale) end

--- Get the forward direction vector (local Z axis in world space).
---@return Vec3
function Transform:GetForward() end

--- Get the right direction vector (local X axis in world space).
---@return Vec3
function Transform:GetRight() end

--- Get the up direction vector (local Y axis in world space).
---@return Vec3
function Transform:GetUp() end

--- Check if the transform proxy is valid.
---@return boolean
function Transform:IsValid() end
