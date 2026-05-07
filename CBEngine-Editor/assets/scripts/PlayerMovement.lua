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

        -- Flamethrower (bubble spray + raycast damage)
        FlameBlueprint      = BlueprintRef(),   -- drag BP_FlameBubble.blueprint here
        FlameSpawnRate      = Float(30.0, 1.0, 120.0),   -- bubbles per second
        FlameConeAngle      = Float(12.0, 0.0, 45.0),    -- spray cone half-angle in degrees
        FlameBubbleScaleMin = Float(0.3, 0.05, 2.0),     -- min initial bubble scale
        FlameBubbleScaleMax = Float(0.7, 0.05, 3.0),     -- max initial bubble scale
        FlameRange          = Float(20.0, 1.0, 200.0),   -- raycast range
        FlameHeatPerSecond  = Float(800.0, 0.0, 5000.0), -- heat injected per second
        FlameDamagePerSec   = Float(15.0, 0.0, 200.0),   -- fire damage per second at hit
        FlameDamageRadius   = Float(0.3, 0.0, 3.0),      -- damage area at hit (0 = single voxel)

        -- Drag the Camera entity here (used for aim direction + muzzle position).
        Camera           = EntityRef(),
    }
}

-- ── Helpers ───────────────────────────────────────────────────────────────────

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

    -- Flamethrower spawn accumulator
    self._flameSpawnAccum = 0.0

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

    self.isGrounded    = (hit ~= nil and hit.normal ~= nil and hit.normal.y >= self.GroundNormalMinY)
    self._groundNormal = (self.isGrounded and hit ~= nil and hit.normal ~= nil) and hit.normal or Vector3(0, 1, 0)

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

    -- When grounded on a slope, compute Y to follow the surface.
    -- +9.81*dt pre-compensates for gravity Jolt will apply this step;
    -- without it vel.y ends up slightly into the slope, and Jolt's contact
    -- projection bleeds XZ speed each frame (causing the speed-loss on ramps).
    local function SlopeY(vx, vz)
        local n = self._groundNormal
        if n.y < 0.99 then
            return -(vx * n.x + vz * n.z) / n.y + 9.81 * dt
        end
        return vel.y
    end

    if hasMove then
        local desiredVel  = wishDir * targetSpeed
        local accel       = self.isGrounded and self.Accel or self.AirAccel
        local dv          = ClampMagnitude(desiredVel - horizVel, accel * dt)
        local nx, nz      = vel.x + dv.x, vel.z + dv.z
        local ny          = self.isGrounded and SlopeY(nx, nz) or vel.y
        rb:SetLinearVelocity(Vector3(nx, ny, nz))
    else
        local drag = self.isGrounded and self.GroundDrag or self.AirDrag
        local k    = Math.Exp(-drag * dt)
        local nx, nz = horizVel.x * k, horizVel.z * k
        local ny     = self.isGrounded and SlopeY(nx, nz) or vel.y
        rb:SetLinearVelocity(Vector3(nx, ny, nz))
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
    -- Skip shooting while PhysicsGrab is holding (left-click = throw instead)
    local grabHolding = PhysicsGrabState and PhysicsGrabState[self._entity:GetUUID()]

    self._fireCooldown  = Math.Max(0.0, self._fireCooldown - dt)

    if Input.IsKeyPressed(Key.MouseLeft) and not grabHolding then
        if self._useFlamethrower then
            -- Flamethrower beam: continuous while held
            self:UpdateFlameBeam(dt)
        elseif self._fireCooldown <= 0.0 then
            self:Shoot()
            self._fireCooldown = self.FireRate
        end
    else
        -- Reset spray accumulator when not firing
        self._flameSpawnAccum = 0.0
    end
end

-- ── Aim helper ───────────────────────────────────────────────────────────────

function PlayerMovement:GetAimOriginAndDirection()
    if self.BulletPoint and self.BulletPoint:IsValid() then
        return self.BulletPoint:GetWorldPosition(), self.BulletPoint:GetForward()
    elseif self.cam and self.cam:IsValid() then
        return self.cam:GetWorldPosition(), self.cam:GetForward()
    else
        return self._entity:GetWorldPosition(), self._entity:GetForward()
    end
end

-- ── Shoot ─────────────────────────────────────────────────────────────────────

function PlayerMovement:Shoot()
    local spawnPos, fireDir = self:GetAimOriginAndDirection()

    if self.UseRaycast then
        -- Hitscan mode: instant raycast damage
        local hit = Physic.Raycast(self._scene, spawnPos, fireDir, self.RaycastRange, Layer.All, self._entity)
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

-- ── Flamethrower (Bubble Spray) ──────────────────────────────────────────────

function PlayerMovement:UpdateFlameBeam(dt)
    local spawnPos, fireDir = self:GetAimOriginAndDirection()

    -- ── Raycast damage (same as old beam) ───────────────────────────────────
    local hit = Physic.Raycast(self._scene, spawnPos, fireDir, self.FlameRange, Layer.All, self._entity)
    if hit and hit.point then
        local hitEntity = hit:GetEntity(self._scene)

        -- Heat at hit point
        Heat.Inject(self._scene, hit.point, self.FlameHeatPerSecond * dt)

        -- Fire damage at hit point
        if self.FlameDamageRadius > 0 then
            VoxelDamage.ApplySphere(self._scene, hit.point, self.FlameDamageRadius, {
                type   = DamageType.Fire,
                amount = self.FlameDamagePerSec * dt,
            })
        elseif hitEntity and hitEntity:IsValid() then
            local entityID = hitEntity:GetUUID()
            if entityID then
                VoxelDamage.ApplyAtWorldPos(self._scene, entityID, hit.point, {
                    type      = DamageType.Fire,
                    amount    = self.FlameDamagePerSec * dt,
                    origin    = hit.point,
                    direction = fireDir,
                })
            end
        end
    end

    -- ── Spawn visual bubbles ────────────────────────────────────────────────
    if self.FlameBlueprint and self.FlameBlueprint:IsValid() then
        self._flameSpawnAccum = self._flameSpawnAccum + dt
        local spawnInterval = 1.0 / Math.Max(self.FlameSpawnRate, 1.0)

        while self._flameSpawnAccum >= spawnInterval do
            self._flameSpawnAccum = self._flameSpawnAccum - spawnInterval
            self:_SpawnFlameBubble(spawnPos, fireDir)
        end
    end
end

-- Flame color palette: bright yellow → orange → red
local FlameColors = {
    Vector3(1.0, 0.95, 0.4),   -- bright yellow
    Vector3(1.0, 0.75, 0.2),   -- golden orange
    Vector3(1.0, 0.5,  0.1),   -- orange
    Vector3(0.95, 0.35, 0.05), -- deep orange
    Vector3(0.85, 0.2,  0.05), -- red-orange
}

function PlayerMovement:_SpawnFlameBubble(origin, baseDir)
    local entities = self._scene:Instantiate(self.FlameBlueprint)
    if not entities or #entities == 0 then return end

    local bubble = entities[1]

    -- Random direction within cone
    local coneRad = Math.Rad(self.FlameConeAngle)
    local randAngle = math.random() * 2.0 * math.pi
    local randRadius = math.random() * coneRad

    -- Build perpendicular axes to baseDir
    local up = Vector3(0, 1, 0)
    if math.abs(baseDir.y) > 0.99 then up = Vector3(1, 0, 0) end
    local right = SafeNormalize(up:Cross(baseDir))
    local realUp = baseDir:Cross(right)

    -- Offset direction within cone
    local offX = Math.Sin(randRadius) * Math.Cos(randAngle)
    local offY = Math.Sin(randRadius) * Math.Sin(randAngle)
    local dir = SafeNormalize(baseDir * Math.Cos(randRadius) + right * offX + realUp * offY)

    -- Position and orient — FlameBubble:OnCreate reads GetForward() to launch itself
    bubble:SetPosition(origin)
    bubble:LookAt(origin + dir * 10.0)

    -- Random scale
    local s = self.FlameBubbleScaleMin + math.random() * (self.FlameBubbleScaleMax - self.FlameBubbleScaleMin)
    bubble:SetScale(Vector3(s, s, s))

    -- Random flame color (clone material so we don't mutate the shared asset)
    local mr = bubble:GetComponent(MeshRenderer)
    if mr then
        local mat = mr:GetMaterial()
        if mat then
            local color = FlameColors[math.random(1, #FlameColors)]
            mat:SetAlbedo(color)
            mat:SetRoughness(1.0)
            mat:SetMetallic(0.0)
        end
    end
end
