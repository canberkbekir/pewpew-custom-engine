-- WallTurret.lua
-- Wall-mounted turret: rotates to track the player, fires when in range + line of sight.
--
-- Entity setup:
--   Static entity placed on wall (no RigidBody, or static RigidBody)
--   ScriptComponent → this script
--   Optional child entity "Muzzle" dragged into MuzzlePoint field
--
-- Drag a TurretBullet blueprint into BulletBlueprint.
-- Drag the Player entity into PlayerRef (or leave empty to auto-find "Player").

WallTurret = {
    __fields = {
        Range          = Float(15.0, 1.0, 100.0),
        FireRate       = Float(1.0,  0.1, 20.0),   -- shots per second
        RotationSpeed  = Float(90.0, 0.0, 720.0),  -- degrees/sec yaw tracking
        AimTolerance   = Float(10.0, 0.0, 45.0),   -- degrees; must be within to fire
        BulletBlueprint = BlueprintRef(),
        MuzzlePoint    = EntityRef(),               -- optional barrel tip entity
        PlayerRef      = EntityRef(),               -- optional; auto-finds "Player" if empty
    }
}

local function SafeNorm(v)
    local l = v:Length()
    if l < 0.0001 then return Vector3(0, 0, 1) end
    return v * (1.0 / l)
end

-- Wrap an angle (degrees) into [-180, 180]
local function WrapAngle(a)
    while a >  180 do a = a - 360 end
    while a < -180 do a = a + 360 end
    return a
end

function WallTurret:OnCreate()
    self._fireCooldown = 0.0
    self._player       = nil
end

function WallTurret:OnUpdate(dt)
    -- Resolve player reference
    if not self._player or not self._player:IsValid() then
        if self.PlayerRef and self.PlayerRef:IsValid() then
            self._player = self.PlayerRef
        else
            self._player = self._scene:FindEntity("Player")
        end
    end

    self._fireCooldown = Math.Max(0.0, self._fireCooldown - dt)

    if not self._player or not self._player:IsValid() then return end

    local myPos     = self._entity:GetWorldPosition()
    -- Aim at player chest height (+0.9) for better accuracy
    local playerPos = self._player:GetWorldPosition() + Vector3(0, 0.9, 0)
    local toPlayer  = playerPos - myPos
    local dist      = toPlayer:Length()

    -- Range check
    if dist > self.Range then return end

    -- ── Yaw tracking ─────────────────────────────────────────────────────────
    local horiz     = Vector3(toPlayer.x, 0, toPlayer.z)
    local targetYaw = Math.Deg(Math.Atan2(horiz.x, horiz.z))
    local curYaw    = self._entity:GetRotation().y
    local yawDelta  = WrapAngle(targetYaw - curYaw)
    local maxStep   = self.RotationSpeed * dt
    local newYaw    = curYaw + Math.Clamp(yawDelta, -maxStep, maxStep)
    self._entity:SetRotation(Vector3(0, newYaw, 0))

    -- ── Line-of-sight check ───────────────────────────────────────────────────
    -- Raycast from turret to player; the very first hit must be the player.
    -- If a wall is in the way it will be hit first and we abort.
    local dir = SafeNorm(toPlayer)
    local hit = Physic.Raycast(self._scene, myPos, dir, dist + 0.5, Layer.All, self._entity)
    if not hit then return end
    local hitEnt = hit:GetEntity(self._scene)
    if not hitEnt or not hitEnt:IsValid() then return end
    if hitEnt:GetUUID() ~= self._player:GetUUID() then return end

    -- ── Aim tolerance ─────────────────────────────────────────────────────────
    if math.abs(yawDelta) > self.AimTolerance then return end

    -- ── Fire ──────────────────────────────────────────────────────────────────
    if self._fireCooldown <= 0.0 then
        self:_Fire(dir)
        self._fireCooldown = 1.0 / Math.Max(self.FireRate, 0.01)
    end
end

function WallTurret:_Fire(dir)
    if not self.BulletBlueprint or not self.BulletBlueprint:IsValid() then
        Log.Warn("WallTurret: no BulletBlueprint assigned")
        return
    end

    local origin = (self.MuzzlePoint and self.MuzzlePoint:IsValid())
        and self.MuzzlePoint:GetWorldPosition()
        or  self._entity:GetWorldPosition()

    local entities = self._scene:Instantiate(self.BulletBlueprint)
    if #entities == 0 then return end

    local bullet = entities[1]
    bullet:SetPosition(origin)
    bullet:LookAt(origin + dir * 100)
end
