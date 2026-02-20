---@meta

--- Proxy object for a MeshRendererComponent. Obtained via entity:GetComponent(MeshRenderer).
---@class MeshRenderer
local MeshRenderer = {}

--- Check if the mesh renderer is visible.
---@return boolean
function MeshRenderer:IsVisible() end

--- Set the visibility of the mesh renderer.
---@param visible boolean
function MeshRenderer:SetVisible(visible) end

--- Check if the mesh renderer proxy is valid.
---@return boolean
function MeshRenderer:IsValid() end
