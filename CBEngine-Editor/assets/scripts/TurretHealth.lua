-- TurretHealth.lua
-- Add this ScriptComponent to any turret entity.
-- PlayerBullet calls other:GetScript(TurretHealth):TakeDamage(damage) on hit.

TurretHealth = {
    __fields = {
        MaxHealth = Float(50.0, 1.0, 1000.0),
    }
}

function TurretHealth:OnCreate()
    self._health = self.MaxHealth
    self._dead   = false
end

function TurretHealth:TakeDamage(amount)
    if self._dead then return end
    self._health = self._health - amount
    Log.Info("Turret HP: " .. math.floor(self._health) .. " / " .. math.floor(self.MaxHealth))
    if self._health <= 0 then
        self:_Die()
    end
end

function TurretHealth:_Die()
    if self._dead then return end
    self._dead = true
    Log.Info("Turret destroyed!")
    self._scene:DestroyEntity(self._entity)
end
