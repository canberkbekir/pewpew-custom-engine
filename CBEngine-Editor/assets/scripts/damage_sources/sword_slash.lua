-- Sword slash damage source
-- Applies directional slice damage along a swing path.
--
-- Usage from any script:
--   local Sword = require("scripts/damage_sources/sword_slash")
--   Sword.Slash(self.scene, swingStart, swingEnd, bladeWidth, sharpness)

local Sword = {}

--- Apply slice damage along a line sweep (approximated as a box query).
---@param scene      Scene   The active scene
---@param swingStart Vector3 Start of the swing arc in world space
---@param swingEnd   Vector3 End of the swing arc in world space
---@param bladeWidth number  Half-width of the blade sweep volume
---@param sharpness  number  Damage multiplier (higher = sharper blade)
function Sword.Slash(scene, swingStart, swingEnd, bladeWidth, sharpness)
    -- Build an AABB enclosing the swing
    local center = (swingStart + swingEnd) * 0.5
    local delta  = swingEnd - swingStart
    local halfLen = delta:Length() * 0.5

    -- Approximate half extents (axis-aligned)
    local halfExtents = Vector3(
        math.max(math.abs(delta.x) * 0.5, bladeWidth),
        math.max(math.abs(delta.y) * 0.5, bladeWidth),
        math.max(math.abs(delta.z) * 0.5, bladeWidth)
    )

    local dir = delta:Normalized()

    local hits = VoxelQuery.Box(scene, center, halfExtents)
    for _, hit in ipairs(hits) do
        VoxelDamage.Apply(scene, hit.EntityID, hit.GridCoord, {
            type      = DamageType.Slice,
            amount    = sharpness * 100.0,
            direction = dir,
        })
    end
end

return Sword
