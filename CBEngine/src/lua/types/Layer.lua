---@meta

--- Physics layer constants and utilities.
--- Use with Entity:GetLayer(), Entity:SetLayer(), and Scene:Raycast() layerMask parameter.
---@class Layer
---@field Default integer Layer 0
---@field Player integer Layer 1
---@field Enemy integer Layer 2
---@field Environment integer Layer 3
---@field Projectile integer Layer 4
---@field Trigger integer Layer 5
---@field IgnoreRaycast integer Layer 6
---@field All integer All layers (0xFFFF)
Layer = {}

--- Create a layer bitmask from one or more layer indices.
--- Example: `Layer.Mask(Layer.Player, Layer.Enemy)` returns a mask matching both layers.
---@vararg integer
---@return integer
function Layer.Mask(...) end
