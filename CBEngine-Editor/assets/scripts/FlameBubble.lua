-- FlameBubble.lua
-- Small "fire particle" entity spawned by the flamethrower.
-- Flies forward, injects heat into the octree, applies fire damage on contact,
-- and self-destructs after a short lifetime.
--
-- Blueprint setup:
--   Entity
--     RigidBody  (dynamic, CCD on, gravity scale low ~0.3, small mass)
--     Collider   (small sphere, trigger recommended)
--     MeshRenderer or VoxelRenderer (tiny red/orange mesh for the "bubble")
--     ScriptComponent -> this script
--
-- The spawner (PlayerMovement) sets position and calls LookAt before OnCreate,
-- so GetForward() already points in the fire direction.

FlameBubble = {
    __fields = {
        Lifetime      = Float(1.2,   0.1, 5.0),
        HeatPerSecond = Float(600.0, 0.0, 2000.0),  -- max temperature this flame can produce (~1000C real flamethrower)
        FireDamage    = Float(5.0,   0.0, 100.0),   -- voxel fire damage per second while touching
        SpawnSafeTime = Float(0.03,  0.0, 0.2),
        SpreadOnHit   = Float(0.15,  0.0, 2.0),     -- damage radius (0=single voxel)
        StickLifetime = Float(0.6,   0.0, 3.0),     -- extra lifetime after sticking to surface
    }
}
FlameBubble.__index = FlameBubble

function FlameBubble:OnCreate()
    self._lifeRemaining = self.Lifetime
    self._safeTimer     = self.SpawnSafeTime
    self._dead          = false
    self._stuck         = false    -- true after first surface hit
    self._stuckTarget   = nil      -- entity we're burning
    self._damageTimer   = 0.0      -- accumulates dt for periodic damage ticks

    -- Connect collision events
    local me = self
    me.Collision.OnCollisionBegin:Connect(function(other, point, normal)
        me:_OnHit(other, point, normal)
    end)
    me.Collision.OnTriggerEnter:Connect(function(other, point, normal)
        me:_OnHit(other, point, normal)
    end)
end

function FlameBubble:OnUpdate(dt)
    if self._dead then return end

    self._lifeRemaining = self._lifeRemaining - dt
    self._safeTimer     = Math.Max(0.0, self._safeTimer - dt)

    -- Inject heat at current position every frame
    local pos = self._entity:GetWorldPosition()
    Heat.Inject(self._scene, pos, self.HeatPerSecond)

    -- While stuck to a surface, apply continuous fire damage
    if self._stuck then
        self._damageTimer = self._damageTimer + dt
        -- Apply damage tick every 0.15 seconds
        if self._damageTimer >= 0.15 then
            self._damageTimer = self._damageTimer - 0.15
            if self.SpreadOnHit > 0 then
                VoxelDamage.ApplySphere(self._scene, pos, self.SpreadOnHit, {
                    type   = DamageType.Fire,
                    amount = self.FireDamage,
                })
            elseif self._stuckTarget and self._stuckTarget:IsValid() then
                local entityID = self._stuckTarget:GetUUID()
                if entityID then
                    VoxelDamage.ApplyAtWorldPos(self._scene, entityID, pos, {
                        type   = DamageType.Fire,
                        amount = self.FireDamage,
                    })
                end
            end
        end
    end

    -- Shrink over lifetime for visual fade-out
    local lifeRatio = Math.Clamp(self._lifeRemaining / self.Lifetime, 0.0, 1.0)
    local scale = lifeRatio * 0.8 + 0.2  -- shrink from 1.0 to 0.2
    self._entity:SetScale(Vector3(scale, scale, scale))

    if self._lifeRemaining <= 0.0 then
        self._dead = true
        self._scene:DestroyEntity(self._entity)
    end
end

function FlameBubble:_OnHit(other, point, normal)
    if self._dead then return end
    if self._safeTimer > 0.0 then return end
    if self._stuck then return end  -- already stuck, ignore further collisions

    -- Ignore other projectiles (same layer) — flame bubbles pass through each other
    if other and other:IsValid() then
        if other:GetLayer() == self._entity:GetLayer() then return end
    end

    -- Stick to surface: stop moving, extend lifetime
    self._stuck = true
    self._lifeRemaining = self._lifeRemaining + self.StickLifetime

    local rb = self._entity:GetComponent(RigidBody)
    if rb then
        rb:SetLinearVelocity(Vector3(0, 0, 0))
        rb:SetUseGravity(false)
    end

    -- Remember what we hit for continuous damage
    if other and other:IsValid() then
        self._stuckTarget = other
    end

    -- Apply initial fire damage burst at impact
    if other and other:IsValid() and point then
        local entityID = other:GetUUID()
        if entityID then
            if self.SpreadOnHit > 0 then
                VoxelDamage.ApplySphere(self._scene, point, self.SpreadOnHit, {
                    type   = DamageType.Fire,
                    amount = self.FireDamage,
                })
            else
                VoxelDamage.ApplyAtWorldPos(self._scene, entityID, point, {
                    type      = DamageType.Fire,
                    amount    = self.FireDamage,
                    origin    = point,
                    direction = normal * -1.0,
                })
            end
        end

        -- Heat burst at impact
        Heat.Inject(self._scene, point, self.HeatPerSecond)
    end
end
