-- Bullet.lua
-- Base bullet class for all projectile scripts.
--
-- USAGE — inherit and override:
--
--   require("Bullet")  -- not needed if engine preloads it
--
--   RocketBullet = Bullet:Extend()
--   RocketBullet.__fields.ExplosionRadius = Float(3, 0, 20)
--
--   function RocketBullet:OnHit(other, point, normal)
--       -- splash damage, particles, etc.
--   end
--
-- SPAWNING — the caller must set position + orientation BEFORE OnCreate fires:
--
--   local entities = self._scene:InstantiateBlueprint(self.BulletBlueprint)
--   local bullet   = entities[1]
--   bullet:SetPosition(muzzlePos)
--   bullet:LookAt(targetPoint)          -- sets forward = shoot direction
--
-- OnCreate reads GetForward() and launches the RigidBody automatically.

Bullet = {
    __fields = {
        Damage   = Float(10,  0, 1000),
        Speed    = Float(50,  1, 500),
        Lifetime = Float(5,   0.1, 30),
    }
}

-- ── Inheritance ──────────────────────────────────────────────────────────────

function Bullet:Extend()
    local child = {}
    child.__fields = {}
    for k, v in pairs(self.__fields) do child.__fields[k] = v end
    setmetatable(child, { __index = self })
    return child
end

-- ── Lifecycle ─────────────────────────────────────────────────────────────────

function Bullet:OnCreate()
    self._lifeRemaining = self.Lifetime

    -- Direction is determined by the entity's orientation set by the spawner.
    self._direction = self._entity:GetForward()

    -- Launch via RigidBody if present.
    local rb = self._entity:GetComponent(RigidBody)
    if rb then
        self._rb = rb
        rb:SetUseGravity(false)
        rb:SetLinearVelocity(self._direction * self.Speed)
    end
end

function Bullet:OnUpdate(dt)
    self._lifeRemaining = self._lifeRemaining - dt
    if self._lifeRemaining <= 0 then
        self._scene:DestroyEntity(self._entity)
    end
end

-- ── Collision ─────────────────────────────────────────────────────────────────

-- Called by both solid and trigger contacts. Calls OnHit, then destroys self.
function Bullet:OnCollision(other, point, normal)
    self:OnHit(other, point, normal)
    self._scene:DestroyEntity(self._entity)
end

function Bullet:OnTriggerEnter(other, point, normal)
    self:OnHit(other, point, normal)
    self._scene:DestroyEntity(self._entity)
end

-- ── Override in child classes ─────────────────────────────────────────────────

--- Called once when the bullet hits something, before self is destroyed.
--- @param other Entity  the entity that was hit (may be invalid for world geometry)
--- @param point Vector3 world-space contact point
--- @param normal Vector3 surface normal at the contact
function Bullet:OnHit(other, point, normal)
    -- Base: do nothing. Override to apply damage, spawn effects, etc.
end

--- Helper so child classes can call self:GetDamage() for final value.
function Bullet:GetDamage()
    return self.Damage
end
