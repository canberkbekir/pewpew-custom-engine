-- TurretBullet.lua
-- Projectile fired by WallTurret.
--
-- Blueprint setup:
--   Entity (Layer: Projectile)
--     RigidBody  (dynamic, CCD on, gravity off recommended)
--     Collider   (small sphere)
--     ScriptComponent → this script
--
-- WallTurret places the bullet at the muzzle and calls LookAt(target)
-- before OnCreate fires, so GetForward() already points at the player.

TurretBullet = {
    __fields = {
        Damage        = Float(10.0, 0.0, 200.0),
        Speed         = Float(25.0, 1.0, 300.0),
        Lifetime      = Float(5.0,  0.1, 30.0),
        SpawnSafeTime = Float(0.1,  0.0, 1.0),   -- ignore collisions right after spawn
    }
}

function TurretBullet:OnCreate()
    self._life = self.Lifetime
    self._safe = self.SpawnSafeTime
    self._dead = false

    self._rb = self._entity:GetComponent(RigidBody)
    if not self._rb then
        Log.Warn("TurretBullet: no RigidBody on bullet entity")
        return
    end

    self._rb:SetLinearVelocity(self._entity:GetForward() * self.Speed)
    self._rb:SetUseGravity(false)
    self._rb:SetAngularDamping(100.0)

    local me = self
    me.Collision.OnCollisionBegin:Connect(function(other, point, normal)
        if me._dead or me._safe > 0 then return end
        me:_OnHit(other, point, normal)
    end)
    me.Collision.OnTriggerEnter:Connect(function(other, point, normal)
        if me._dead or me._safe > 0 then return end
        me:_OnHit(other, point, normal)
    end)
end

function TurretBullet:OnUpdate(dt)
    if self._dead then return end
    if self._safe > 0 then self._safe = self._safe - dt end
    self._life = self._life - dt
    if self._life <= 0 then self:_Die() end
end

function TurretBullet:_OnHit(other, point, normal)
    if other and other:IsValid() then
        local health = other:GetScript(PlayerHealth)
        if health then
            health:TakeDamage(self.Damage)
        end
    end
    self:_Die()
end

function TurretBullet:_Die()
    if self._dead then return end
    self._dead = true
    self._scene:DestroyEntity(self._entity)
end
