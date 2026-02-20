---@meta

--- Debug drawing functions for visualizing lines and rays in the scene.
---@class Debug
Debug = {}

--- Draw a line between two points.
---@param from Vec3 Start position
---@param to Vec3 End position
---@param color? Vec3 RGB color (default: green 0,1,0)
---@param duration? number How long to display in seconds (default: 0, single frame)
function Debug.DrawLine(from, to, color, duration) end

--- Draw a ray from an origin in a direction.
---@param origin Vec3 Start position
---@param direction Vec3 Ray direction
---@param maxDistance? number Length of the ray (default: 100)
---@param color? Vec3 RGB color (default: red 1,0,0)
---@param duration? number How long to display in seconds (default: 0, single frame)
function Debug.DrawRay(origin, direction, maxDistance, color, duration) end
