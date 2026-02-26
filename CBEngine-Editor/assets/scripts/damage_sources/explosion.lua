-- Explosion damage source
-- Applies radial explosion damage to all destructible voxels in a sphere.
--
-- Usage from any script:
--   local Explosion = require("scripts/damage_sources/explosion")
--   Explosion.Apply(self.scene, position, radius, power)

local Explosion = {}

--- Apply explosion damage with quadratic falloff.
---@param scene   Scene   The active scene
---@param origin  Vector3 Center of the explosion in world space
---@param radius  number  Blast radius
---@param power   number  Base damage at the explosion center
function Explosion.Apply(scene, origin, radius, power)
    local hits = VoxelQuery.Sphere(scene, origin, radius)
    for _, hit in ipairs(hits) do
        local dist = (hit.WorldPos - origin):Length()
        local falloff = (1.0 - dist / radius) ^ 2  -- quadratic falloff

        VoxelDamage.Apply(scene, hit.EntityID, hit.GridCoord, {
            type   = DamageType.Explosion,
            amount = power * falloff,
            origin = origin,
        })
    end
end

return Explosion
