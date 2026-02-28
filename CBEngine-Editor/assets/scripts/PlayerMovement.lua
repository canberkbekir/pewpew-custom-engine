-- PlayerMovement.lua
-- FPS controller: yaw/pitch look, movement, jump, and shooting.
--
-- Entity hierarchy expected:
--   Player (this script + RigidBody)
--     └── CameraPoint  (pitch pivot)
--           └── Camera

PlayerMovement = {
    __fields = {
        -- Movement
        MoveSpeed        = Float(6.5,  0, 30),
        SprintSpeed      = Float(9.5,  0, 40),
        Accel            = Float(35.0, 0, 200),
        AirAccel         = Float(10.0, 0, 100),
        MaxSpeed         = Float(12.0,  0, 50),
        GroundDrag       = Float(10.0, 0, 40),
        AirDrag          = Float(0.5,  0, 10),

        -- Jump
        JumpImpulse      = Float(6.5, 0, 50),
        GroundCheckDist  = Float(1.25, 0, 5),
        GroundRayStartUp = Float(0.25, 0, 2),
        GroundNormalMinY = Float(0.55, 0, 1),
        UseJumpBuffer    = Bool(true),
        JumpBufferTime   = Float(0.12, 0, 0.5),
        UseCoyoteTime    = Bool(true),
        CoyoteTime       = Float(0.10, 0, 0.5),
        EnableSprint     = Bool(true),

        -- Look (sensitivity is degrees per pixel)
        MouseSensitivity  = Float(0.1, 0.01, 5.0),
        PitchMin         = Float(-85.0, -89, 0),
        PitchMax         = Float(85.0,   0, 89),
        LookSmoothTime   = Float(0.05, 0.0, 0.3), 

        -- Shooting
        BulletBlueprint  = BlueprintRef(),   -- drag .blueprint here
        BulletPoint      = EntityRef(),      -- muzzle transform (child of gun/camera)
        FireRate         = Float(0.15, 0.01, 5.0),
        UseRaycast       = Bool(false),       -- true = hitscan raycast, false = spawn bullet
        RaycastDamage    = Float(25.0, 0.0, 1000.0),
        RaycastRange     = Float(100.0, 1.0, 500.0),
        RaycastDamageRadius = Float(0.3, 0.0, 5.0), -- 0 = single voxel, >0 = crater

        -- Flamethrower (beam mode)
        FlameBlueprint     = BlueprintRef(),   -- drag BP_FlameBeam.blueprint here
        FlameBeamLength    = Float(20.0, 1.0, 200.0),   -- max beam range
        FlameBeamWidth     = Float(0.15, 0.01, 2.0),    -- visual beam thickness
        FlameHeatPerSecond = Float(800.0, 0.0, 5000.0), -- heat injected along beam per second
        FlameDamagePerSec  = Float(15.0, 0.0, 200.0),   -- fire damage per second at hit point
        FlameDamageRadius  = Float(0.3, 0.0, 3.0),      -- damage area at hit point (0 = single voxel)
        FlameHeatPoints    = Int(5, 1, 20),              -- heat injection points along beam

        -- Drag the Camera entity here (used for aim direction + muzzle position).
        Camera           = EntityRef(),
    }
}

-- ── Helpers ───────────────────────────────────────────────────────────────────

local function FlattenXZ(v) return Vector3(v.x, 0, v.z) end

local function SafeNormalize(v)
    local len = v:Length()
    if len > 0.0001 then return v:Normalized() end
    return Vector3(0, 0, 0)
end

local function YawBasis(yawRad)
    local cy = Math.Cos(yawRad)
    local sy = Math.Sin(yawRad)
    return Vector3(sy, 0, cy),   -- forward
           Vector3(cy, 0, -sy)   -- right
end

local function ClampMagnitude(v, maxLen)
    local len = v:Length()
    if len > maxLen and len > 0.0001 then
        return v * (maxLen / len)
    end
    return v
end

local function ExpAlpha(dt, smoothTime)
    if smoothTime <= 0.0001 then return 1.0 end
    return 1.0 - Math.Exp(-dt / smoothTime)
end

-- ── Lifecycle ─────────────────────────────────────────────────────────────────

function PlayerMovement:OnCreate()
    local entity = self._entity

    -- Look state — all angles stored in DEGREES.
    -- SetRotation/GetRotation now take/return degrees (same convention as the editor).
    local rot = entity:GetRotation()   -- returns degrees
    self.yawTarget     = rot.y
    self.pitchTarget   = 0.0
    self.yawSmoothed   = rot.y
    self.pitchSmoothed = 0.0
    self.isGrounded    = false
    self._coyoteT      = 0.0
    self._jumpBufT     = 0.0
    self._fireCooldown = 0.0
    self.cam           = nil
    self.pitchNode     = nil

    -- Damage type (switchable with 1/2/3/4 keys)
    self._currentDamageType = DamageType.Impact
    self._damageTypeName    = "Impact"
    self._useFlamethrower   = false
    self._flameCooldown     = 0.0

    -- Beam entity (lazy-spawned on first fire, reused)
    self._beamEntity = nil
    self._beamActive = false

    self.rb = entity:GetComponent(RigidBody)
    if not self.rb then
        Log.Error("PlayerMovement: needs RigidBody")
        return
    end
    Input.SetCursorLocked(true)

    -- Resolve camera entity
    if self.Camera and self.Camera:IsValid() then
        self.cam = self.Camera
    else
        for _, child in ipairs(entity:GetChildren()) do
            if child:GetComponent(Camera) then
                self.cam = child
                break
            end
        end
    end

    -- Resolve pitch pivot (intermediate parent between player and camera)
    -- Priority: Camera's parent (if not the player), else a child named CameraPoint/PitchPivot, else camera.
    if self.cam and self.cam:IsValid() then
        if self.cam:HasParent() then
            local p = self.cam:GetParent()
            if p and p:IsValid() and p:GetUUID() ~= entity:GetUUID() then
                self.pitchNode = p
            end
        end
        if not self.pitchNode then
            for _, child in ipairs(entity:GetChildren()) do
                local n = child:GetName()
                if n == "CameraPoint" or n == "PitchPivot" then
                    self.pitchNode = child
                    break
                end
            end
        end
        if not self.pitchNode then
            self.pitchNode = self.cam
        end
    end
end

function PlayerMovement:OnUpdate(dt)
    local entity = self._entity
    local scene  = self._scene
    local rb     = self.rb
    if not rb then return end
    dt = Math.Max(dt, 0.0001)

    -- Toggle cursor lock: Esc to release, left-click to re-lock.
    if Input.IsKeyJustPressed(Key.Escape) then
        Input.SetCursorLocked(false)
    elseif Input.IsKeyJustPressed(Key.MouseLeft) and not Input.IsCursorLocked() then
        Input.SetCursorLocked(true)
    end

    -- ── Look ──────────────────────────────────────────────────────────────────
    -- Angles are degrees. SetRotation/GetRotation use degrees (matches the editor).
    local md = Input.IsCursorLocked() and Input.GetMouseDelta() or Vector2(0, 0)
    self.yawTarget   = self.yawTarget   - md.x * self.MouseSensitivity
    self.pitchTarget = Math.Clamp(
        self.pitchTarget + md.y * self.MouseSensitivity,
        self.PitchMin, self.PitchMax)

    local a = ExpAlpha(dt, self.LookSmoothTime)
    self.yawSmoothed   = self.yawSmoothed   + (self.yawTarget   - self.yawSmoothed)   * a
    self.pitchSmoothed = Math.Clamp(
        self.pitchSmoothed + (self.pitchTarget - self.pitchSmoothed) * a,
        self.PitchMin, self.PitchMax)

    -- Pass degrees directly — SetRotation now expects degrees
    entity:SetRotation(Vector3(0.0, self.yawSmoothed, 0.0))
    if self.pitchNode and self.pitchNode:IsValid() then
        self.pitchNode:SetLocalRotation(Vector3(self.pitchSmoothed, 0.0, 0.0))
    end

    -- ── Ground check ──────────────────────────────────────────────────────────
    local pos      = entity:GetWorldPosition()
    local rayStart = pos + Vector3(0, self.GroundRayStartUp, 0)
    local hit      = Physic.Raycast(scene, rayStart, Vector3(0, -1, 0),
        self.GroundCheckDist, Layer.All, entity)

    self.isGrounded = (hit ~= nil and hit.normal ~= nil and hit.normal.y >= self.GroundNormalMinY)

    if self.isGrounded then
        self._coyoteT = self.CoyoteTime
    elseif self.UseCoyoteTime then
        self._coyoteT = Math.Max(0.0, self._coyoteT - dt)
    else
        self._coyoteT = 0.0
    end

    -- ── Movement ──────────────────────────────────────────────────────────────
    -- W/S = forward/back, D/A = strafe right/left
    local move = Vector3(0, 0, 0)
    if Input.IsKeyPressed(Key.W) then move = move + Vector3(0, 0, 1) end
    if Input.IsKeyPressed(Key.S) then move = move - Vector3(0, 0, 1) end
    if Input.IsKeyPressed(Key.D) then move = move - Vector3(1, 0, 0) end
    if Input.IsKeyPressed(Key.A) then move = move + Vector3(1, 0, 0) end

    local hasMove = move:Length() > 0.01

    -- Derive forward/right from yawSmoothed (always current-frame, no WorldMatrix lag)
    local forward, right = YawBasis(Math.Rad(self.yawSmoothed))

    local wishDir = SafeNormalize(forward * move.z + right * move.x)

    local targetSpeed = self.MoveSpeed
    if self.EnableSprint and Input.IsKeyPressed(Key.LeftShift) then
        targetSpeed = self.SprintSpeed
    end
    targetSpeed = Math.Min(targetSpeed, self.MaxSpeed)

    local vel      = rb:GetLinearVelocity()
    local horizVel = Vector3(vel.x, 0, vel.z)

    if hasMove then
        local desiredVel  = wishDir * targetSpeed
        local accel       = self.isGrounded and self.Accel or self.AirAccel
        local dv          = ClampMagnitude(desiredVel - horizVel, accel * dt)
        rb:SetLinearVelocity(Vector3(vel.x + dv.x, vel.y, vel.z + dv.z))
    else
        local drag = self.isGrounded and self.GroundDrag or self.AirDrag
        local k    = Math.Exp(-drag * dt)
        rb:SetLinearVelocity(Vector3(horizVel.x * k, vel.y, horizVel.z * k))
    end

    -- ── Jump ──────────────────────────────────────────────────────────────────
    if self.UseJumpBuffer then
        if Input.IsKeyJustPressed(Key.Space) then
            self._jumpBufT = self.JumpBufferTime
        else
            self._jumpBufT = Math.Max(0.0, self._jumpBufT - dt)
        end
    else
        self._jumpBufT = Input.IsKeyJustPressed(Key.Space) and 0.001 or 0.0
    end

    local canJump = self.isGrounded or (self.UseCoyoteTime and self._coyoteT > 0.0)
    if self._jumpBufT > 0.0 and canJump then
        local v = rb:GetLinearVelocity()
        if v.y < 0.0 then rb:SetLinearVelocity(Vector3(v.x, 0.0, v.z)) end
        rb:AddImpulse(Vector3(0, self.JumpImpulse, 0))
        self._jumpBufT  = 0.0
        self._coyoteT   = 0.0
        self.isGrounded = false
    end

    -- ── Weapon/Damage type switch (1/2/3/4) ────────────────────────────────────
    if Input.IsKeyJustPressed(Key.Num1) then
        self._currentDamageType = DamageType.Impact
        self._damageTypeName    = "Impact"
        self._useFlamethrower   = false
        Log.Info("Damage type: Impact")
    elseif Input.IsKeyJustPressed(Key.Num2) then
        self._currentDamageType = DamageType.Fire
        self._damageTypeName    = "Fire"
        self._useFlamethrower   = false
        Log.Info("Damage type: Fire")
    elseif Input.IsKeyJustPressed(Key.Num3) then
        self._currentDamageType = DamageType.Explosion
        self._damageTypeName    = "Explosion"
        self._useFlamethrower   = false
        Log.Info("Damage type: Explosion")
    elseif Input.IsKeyJustPressed(Key.Num4) then
        self._useFlamethrower   = true
        self._damageTypeName    = "Flamethrower"
        Log.Info("Weapon: Flamethrower")
    end

    -- ── Shoot / Flamethrower ─────────────────────────────────────────────────
    self._fireCooldown  = Math.Max(0.0, self._fireCooldown - dt)

    if Input.IsKeyPressed(Key.MouseLeft) then
        if self._useFlamethrower then
            -- Flamethrower beam: continuous while held
            self:UpdateFlameBeam(dt)
        elseif self._fireCooldown <= 0.0 then
            self:Shoot()
            self._fireCooldown = self.FireRate
        end
    else
        -- Hide beam when not firing
        if self._beamActive then
            self._beamActive = false
            if self._beamEntity and self._beamEntity:IsValid() then
                self._beamEntity:SetVisible(false)
            end
        end
    end
end

-- ── Shoot ─────────────────────────────────────────────────────────────────────

function PlayerMovement:Shoot()
    -- Determine origin and direction from BulletPoint, Camera, or player root
    local spawnPos, fireDir
    if self.BulletPoint and self.BulletPoint:IsValid() then
        spawnPos = self.BulletPoint:GetWorldPosition()
        fireDir  = self.BulletPoint:GetForward()
    elseif self.cam and self.cam:IsValid() then
        spawnPos = self.cam:GetWorldPosition()
        fireDir  = self.cam:GetForward()
    else
        spawnPos = self._entity:GetWorldPosition()
        fireDir  = self._entity:GetForward()
    end

    if self.UseRaycast then
        -- Hitscan mode: instant raycast damage
        local hit = Physic.Raycast(self._scene, spawnPos, fireDir, self.RaycastRange)
        if hit then
            local hitEntity = hit:GetEntity(self._scene)

            -- Voxel damage at hit point
            if hit.point then
                if self.RaycastDamageRadius > 0 then
                    VoxelDamage.ApplySphere(self._scene, hit.point, self.RaycastDamageRadius, {
                        type   = self._currentDamageType,
                        amount = self.RaycastDamage,
                    })
                else
                    if hitEntity and hitEntity:IsValid() then
                        local entityID = hitEntity:GetUUID()
                        if entityID then
                            VoxelDamage.ApplyAtWorldPos(self._scene, entityID, hit.point, {
                                type   = self._currentDamageType,
                                amount = self.RaycastDamage,
                                origin = hit.point,
                                direction = hit.normal * -1.0,
                            })
                        end
                    end
                end
            end

            -- Break hinge joints if hit
            if hitEntity and hitEntity:IsValid() then
                local joint = hitEntity:GetComponent(HingeJoint)
                if joint and joint:IsActive() then
                    joint:Break()
                end
            end
        end
    else
        -- Bullet mode: spawn projectile entity
        if not self.BulletBlueprint or not self.BulletBlueprint:IsValid() then
            Log.Warn("PlayerMovement: no BulletBlueprint assigned")
            return
        end

        local entities = self._scene:Instantiate(self.BulletBlueprint)
        if #entities == 0 then
            Log.Warn("PlayerMovement: Instantiate returned no entities")
            return
        end

        local bullet = entities[1]
        bullet:SetPosition(spawnPos)
        bullet:LookAt(spawnPos + fireDir * 100)
    end
end

-- ── Flamethrower (Beam Mode) ──────────────────────────────────────────────────

function PlayerMovement:UpdateFlameBeam(dt)
    -- Determine origin and direction
    local spawnPos, fireDir
    if self.BulletPoint and self.BulletPoint:IsValid() then
        spawnPos = self.BulletPoint:GetWorldPosition()
        fireDir  = self.BulletPoint:GetForward()
    elseif self.cam and self.cam:IsValid() then
        spawnPos = self.cam:GetWorldPosition()
        fireDir  = self.cam:GetForward()
    else
        spawnPos = self._entity:GetWorldPosition()
        fireDir  = self._entity:GetForward()
    end

    -- Raycast to find hit point (or use max beam length)
    local beamLength = self.FlameBeamLength
    local hitPoint   = nil
    local hitEntity  = nil

    local hit = Physic.Raycast(self._scene, spawnPos, fireDir, beamLength, Layer.All, self._entity)
    if hit and hit.point then
        local diff = hit.point - spawnPos
        beamLength = diff:Length()
        hitPoint   = hit.point
        hitEntity  = hit:GetEntity(self._scene)
    end

    -- Lazy-spawn beam entity on first use
    if not self._beamEntity or not self._beamEntity:IsValid() then
        if self.FlameBlueprint and self.FlameBlueprint:IsValid() then
            local entities = self._scene:Instantiate(self.FlameBlueprint)
            if entities and #entities > 0 then
                self._beamEntity = entities[1]
            end
        end
        if not self._beamEntity then
            Log.Warn("PlayerMovement: no FlameBlueprint assigned (drag BP_FlameBeam)")
            return
        end
    end

    -- Position beam: centered between spawn and end, oriented along fireDir
    local beamEnd    = spawnPos + fireDir * beamLength
    local beamCenter = spawnPos + fireDir * (beamLength * 0.5)
    self._beamEntity:SetPosition(beamCenter)
    self._beamEntity:LookAt(beamEnd)
    self._beamEntity:SetScale(Vector3(self.FlameBeamWidth, self.FlameBeamWidth, beamLength))
    self._beamEntity:SetVisible(true)
    self._beamActive = true

    -- Inject heat at multiple points along the beam
    local heatPerPoint = self.FlameHeatPerSecond / Math.Max(self.FlameHeatPoints, 1)
    for i = 0, self.FlameHeatPoints - 1 do
        local t = (i + 0.5) / self.FlameHeatPoints
        local p = spawnPos + fireDir * (beamLength * t)
        Heat.Inject(self._scene, p, heatPerPoint)
    end

    -- Apply fire damage at hit point
    if hitPoint then
        if self.FlameDamageRadius > 0 then
            VoxelDamage.ApplySphere(self._scene, hitPoint, self.FlameDamageRadius, {
                type   = DamageType.Fire,
                amount = self.FlameDamagePerSec * dt,
            })
        elseif hitEntity and hitEntity:IsValid() then
            local entityID = hitEntity:GetUUID()
            if entityID then
                VoxelDamage.ApplyAtWorldPos(self._scene, entityID, hitPoint, {
                    type      = DamageType.Fire,
                    amount    = self.FlameDamagePerSec * dt,
                    origin    = hitPoint,
                    direction = fireDir,
                })
            end
        end
    end
end
