---@meta

--- Proxy object for a DirectionalLightComponent. Obtained via entity:GetComponent(DirectionalLight).
---@class DirectionalLight
local DirectionalLight = {}

--- Get the light color.
---@return Vec3
function DirectionalLight:GetColor() end

--- Set the light color.
---@param color Vec3
function DirectionalLight:SetColor(color) end

--- Get the light intensity.
---@return number
function DirectionalLight:GetIntensity() end

--- Set the light intensity.
---@param intensity number
function DirectionalLight:SetIntensity(intensity) end

--- Check if the light casts shadows.
---@return boolean
function DirectionalLight:IsCastingShadows() end

--- Set whether the light casts shadows.
---@param cast boolean
function DirectionalLight:SetCastShadows(cast) end

--- Check if the light is visible.
---@return boolean
function DirectionalLight:IsVisible() end

--- Set the visibility of the light.
---@param visible boolean
function DirectionalLight:SetVisible(visible) end

--- Check if the directional light proxy is valid.
---@return boolean
function DirectionalLight:IsValid() end
