-- PlayerHealth.lua
-- Add this ScriptComponent to the Player entity.
-- TurretBullet calls self:GetScript(PlayerHealth):TakeDamage(amount) on hit.

PlayerHealth = {
    __fields = {
        MaxHealth      = Float(100.0, 1.0, 1000.0),
        InvincibleTime = Float(0.5,   0.0, 5.0),   -- iframe window after each hit
    }
}

function PlayerHealth:OnCreate()
    self._health = self.MaxHealth
    self._iframeT = 0.0
    self._dead    = false
    Log.Info("PlayerHealth: initialised — " .. self.MaxHealth .. " HP")
end

function PlayerHealth:OnUpdate(dt)
    if self._iframeT > 0 then
        self._iframeT = self._iframeT - dt
    end
end

function PlayerHealth:TakeDamage(amount)
    if self._dead          then return end
    if self._iframeT > 0   then return end
    self._health  = self._health - amount
    self._iframeT = self.InvincibleTime
    Log.Info("Player HP: " .. math.floor(self._health) .. " / " .. math.floor(self.MaxHealth))
    if self._health <= 0 then
        self:_OnDeath()
    end
end

function PlayerHealth:Heal(amount)
    self._health = Math.Min(self._health + amount, self.MaxHealth)
    Log.Info("Player HP: " .. math.floor(self._health) .. " / " .. math.floor(self.MaxHealth))
end

function PlayerHealth:GetHealthFraction()
    return self._health / self.MaxHealth
end

function PlayerHealth:_OnDeath()
    self._dead = true
    Log.Info("Player died — closing play mode")
    Application_Quit()
end
