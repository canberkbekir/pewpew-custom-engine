-- PlayerBullet.lua
-- Projectile spawned by PlayerMovement:Shoot().
--
-- Blueprint setup:
--   Entity
--     RigidBody  (dynamic, CCD on, gravity off recommended)
--     Collider   (small sphere/capsule)
--     ScriptComponent → this script
--
-- PlayerMovement places the bullet at the muzzle and calls LookAt(targetPoint)
-- before OnCreate fires, so GetForward() already points at the target.

PlayerBullet = {
    __fields = {
        Damage        = Float(10.0,  0.0, 1000.0),
        DamageRadius  = Float(0.3,   0.0, 5.0),    -- voxel damage radius (0 = single voxel)
        Speed         = Float(50.0,  1.0, 500.0),
        Lifetime      = Float(4.0,   0.1, 30.0),
        UseGravity    = Bool(false),               -- false = hitscan-style flat arc
        SpawnSafeTime = Float(0.05,  0.0, 0.5),   -- ignore collisions for this long after spawn
    }
}

-- ── Lifecycle ──────────────────────────────────────────────────────────────────

function PlayerBullet:OnCreate()
    self._lifeRemaining = self.Lifetime
    self._safeTimer     = self.SpawnSafeTime
    self._dead          = false

    self._rb = self._entity:GetComponent(RigidBody)
    if not self._rb then
        Log.Warn("PlayerBullet: no RigidBody found on bullet entity")
        return
    end

    -- PlayerMovement:LookAt(targetPoint) already set our rotation,
    -- so forward points straight at the target.
    local dir = self._entity:GetForward()
    self._rb:SetLinearVelocity(dir * self.Speed)
    self._rb:SetUseGravity(self.UseGravity)
    self._rb:SetAngularDamping(100.0)  -- no spinning

    local me = self
    me.Collision.OnCollisionBegin:Connect(function(other, point, normal)
        if me._dead then return end
        if me._safeTimer > 0 then return end
        me:_OnHit(other, point, normal)
    end)
    me.Collision.OnTriggerEnter:Connect(function(other, point, normal)
        if me._dead then return end
        if me._safeTimer > 0 then return end
        me:_OnHit(other, point, normal)
    end)
end

function PlayerBullet:OnUpdate(dt)
    if self._dead then return end

    -- Spawn safe window: don't allow collision responses right after spawn
    if self._safeTimer > 0 then
        self._safeTimer = self._safeTimer - dt
    end

    -- Lifetime expiry
    self._lifeRemaining = self._lifeRemaining - dt
    if self._lifeRemaining <= 0 then
        self:_Die()
    end
end

-- ── Hit logic ──────────────────────────────────────────────────────────────────

function PlayerBullet:_OnHit(other, point, normal)
    if other and other:IsValid() then

        -- Generic health damage
        if Health then
            local health = other:GetComponent(Health)
            if health and health.TakeDamage then
                health:TakeDamage(self.Damage)
            end
        end

        -- Voxel damage: apply impact at the collision point
        if point then
            if self.DamageRadius > 0 then
                -- Area damage: damages all voxels in radius with distance falloff
                VoxelDamage.ApplySphere(self._scene, point, self.DamageRadius, {
                    type   = DamageType.Impact,
                    amount = self.Damage,
                })
            else
                -- Single voxel damage
                local entityID = other:GetUUID()
                if entityID then
                    VoxelDamage.ApplyAtWorldPos(self._scene, entityID, point, {
                        type   = DamageType.Impact,
                        amount = self.Damage,
                        origin = point,
                        direction = normal * -1.0,
                    })
                end
            end
        end

        -- Chain link hit: break the joint so the link and everything below it
        -- detaches from the anchor and falls as a connected segment.
        local joint = other:GetComponent(HingeJoint)
        if joint and joint:IsActive() then
            joint:Break()
        end

    end

    self:_Die()
end

function PlayerBullet:_Die()
    if self._dead then return end
    self._dead = true
    self._scene:DestroyEntity(self._entity)
end
