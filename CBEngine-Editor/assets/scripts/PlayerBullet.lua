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
        Speed         = Float(50.0,  1.0, 500.0),
        Lifetime      = Float(4.0,   0.1, 30.0),
        GravityScale  = Float(0.0,   0.0, 1.0),   -- 0 = hitscan-style flat arc
        SpawnSafeTime = Float(0.05,  0.0, 0.5),   -- ignore collisions for this long after spawn
    }
}
PlayerBullet.__index = PlayerBullet

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
    self._rb:SetGravityScale(self.GravityScale)
    self._rb:SetAngularDamping(100.0)  -- no spinning
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

-- ── Collision ──────────────────────────────────────────────────────────────────

function PlayerBullet:OnCollisionBegin(other, point, normal)
    if self._dead then return end
    if self._safeTimer > 0 then return end   -- still inside spawn-safe window

    self:_OnHit(other, point, normal)
end

function PlayerBullet:OnTriggerEnter(other, point, normal)
    if self._dead then return end
    if self._safeTimer > 0 then return end

    self:_OnHit(other, point, normal)
end

-- ── Hit logic ──────────────────────────────────────────────────────────────────

function PlayerBullet:_OnHit(other, point, normal)
    -- Try to deal damage if the target has a Health script.
    -- Health script is expected to expose:  function Health:TakeDamage(amount) end
    if other and other:IsValid() then
        if Health then   -- only attempt if a Health class is loaded
            local health = other:GetComponent(Health)
            if health and health.TakeDamage then
                health:TakeDamage(self.Damage)
            end
        end
    end

    self:_Die()
end

function PlayerBullet:_Die()
    if self._dead then return end
    self._dead = true
    self._scene:DestroyEntity(self._entity)
end
