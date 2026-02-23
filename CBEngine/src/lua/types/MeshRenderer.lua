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

--- Get the material proxy for this mesh renderer. Returns nil if no material is assigned.
---@return Material|nil
function MeshRenderer:GetMaterial() end

--- Get the mesh proxy for this mesh renderer. Returns nil if no mesh is assigned.
---@return Mesh|nil
function MeshRenderer:GetMesh() end

--- Check if the mesh renderer proxy is valid.
---@return boolean
function MeshRenderer:IsValid() end
