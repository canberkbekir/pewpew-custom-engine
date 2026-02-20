---@meta

--- Proxy object for a CameraComponent. Obtained via entity:GetComponent(Camera).
---@class Camera
local Camera = {}

--- Get the field of view (degrees).
---@return number
function Camera:GetFOV() end

--- Set the field of view (degrees).
---@param fov number
function Camera:SetFOV(fov) end

--- Get the near clip distance.
---@return number
function Camera:GetNearClip() end

--- Set the near clip distance.
---@param nearClip number
function Camera:SetNearClip(nearClip) end

--- Get the far clip distance.
---@return number
function Camera:GetFarClip() end

--- Set the far clip distance.
---@param farClip number
function Camera:SetFarClip(farClip) end

--- Check if this is the primary (active) camera.
---@return boolean
function Camera:IsPrimary() end

--- Set whether this is the primary (active) camera.
---@param primary boolean
function Camera:SetPrimary(primary) end

--- Check if the camera proxy is valid.
---@return boolean
function Camera:IsValid() end
