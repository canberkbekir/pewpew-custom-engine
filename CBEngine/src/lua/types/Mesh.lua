---@meta

--- Proxy object for a Mesh asset. Obtained via MeshRenderer:GetMesh().
---@class Mesh
local Mesh = {}

--- Check if the mesh proxy is valid (mesh asset is loaded).
---@return boolean
function Mesh:IsValid() end

--- Get the number of vertices in the mesh.
---@return integer
function Mesh:GetVertexCount() end

--- Get the number of indices in the mesh.
---@return integer
function Mesh:GetIndexCount() end

--- Get the minimum corner of the mesh's axis-aligned bounding box (local space).
---@return Vec3
function Mesh:GetBoundsMin() end

--- Get the maximum corner of the mesh's axis-aligned bounding box (local space).
---@return Vec3
function Mesh:GetBoundsMax() end
