-- Fire spread damage source
-- Applies tick-based fire damage in a radius. Call once per tick (not every frame).
--
-- Usage from any script:
--   local Fire = require("scripts/damage_sources/fire_spread")
--   Fire.Tick(self.scene, origin, radius, tickDamage, deltaTime)

local Fire = {}

--- Apply fire damage to all voxels within a radius.
---@param scene      Scene   The active scene
---@param origin     Vector3 Center of the fire in world space
---@param radius     number  Fire effect radius
---@param tickDamage number  Damage per second at full intensity
---@param deltaTime  number  Frame delta time (for damage scaling)
function Fire.Tick(scene, origin, radius, tickDamage, deltaTime)
    local hits = VoxelQuery.Sphere(scene, origin, radius)
    for _, hit in ipairs(hits) do
        local dist = (hit.WorldPos - origin):Length()
        local falloff = 1.0 - (dist / radius)

        VoxelDamage.Apply(scene, hit.EntityID, hit.GridCoord, {
            type   = DamageType.Fire,
            amount = tickDamage * falloff * deltaTime,
            origin = origin,
        })
    end
end

return Fire
