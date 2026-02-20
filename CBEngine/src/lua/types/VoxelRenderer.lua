---@meta

--- Proxy object for a VoxelRendererComponent. Obtained via entity:GetComponent(VoxelRenderer).
---@class VoxelRenderer
local VoxelRenderer = {}

--- Check if the voxel renderer is visible.
---@return boolean
function VoxelRenderer:IsVisible() end

--- Set the visibility of the voxel renderer.
---@param visible boolean
function VoxelRenderer:SetVisible(visible) end

--- Check if the voxel renderer proxy is valid.
---@return boolean
function VoxelRenderer:IsValid() end
