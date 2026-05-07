LevelManager = GameManager:Extend()

LevelManager.__fields = {
    KillY     = Float(-10, -1000, 100),
    PlayerRef = EntityRef(),
}

local CHECKPOINTS = {
    [1] = Vector3(6,  1.5, 14.5),
    [2] = Vector3(5,  1.5, 27),
    [3] = Vector3(8,  1.5, 48),
    [4] = Vector3(7,  1.5, 61),
}

function LevelManager:OnCreate()
    self.currentSection = 0
    self.won = false
    self._player = self.PlayerRef
    if self._player and self._player:IsValid() then
        self.currentSpawn = self._player:GetWorldPosition()
    else
        self.currentSpawn = Vector3(5, 1.5, 2)
    end
    Log.Info("=== Flamethrower Puzzle — Press 4 to equip flame, hold LMB to burn! ===")
end

function LevelManager:OnUpdate(dt)
    if not self._player or not self._player:IsValid() then return end
    local pos = self._player:GetWorldPosition()
    if pos.y < self.KillY then
        self:RespawnPlayer()
    end
end

function LevelManager:RespawnPlayer()
    self._player:SetPosition(self.currentSpawn)
    local rb = self._player:GetComponent(RigidBody)
    if rb then
        rb:SetLinearVelocity(Vector3(0, 0, 0))
    end
end

function LevelManager:SetCheckpoint(pos)
    self.currentSpawn = pos
end

function LevelManager:OnSectionEntered(id)
    if self.won then return end
    self.currentSection = id
    local cp = CHECKPOINTS[id]
    if cp then
        self:SetCheckpoint(cp)
        Log.Info("--- Section " .. id .. " entered — checkpoint set ---")
    else
        Log.Info("--- Section " .. id .. " entered ---")
    end
end

function LevelManager:OnLevelComplete()
    if self.won then return end
    self.won = true
    Log.Info("=== YOU WIN! You burned your way through! ===")
end

function LevelManager:OnDestroy()
end
