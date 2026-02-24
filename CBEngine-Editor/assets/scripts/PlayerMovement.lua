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

    -- ── Shoot ─────────────────────────────────────────────────────────────────
    self._fireCooldown = Math.Max(0.0, self._fireCooldown - dt)
    if Input.IsKeyJustPressed(Key.MouseLeft) and self._fireCooldown <= 0.0 then
        self:Shoot()
        self._fireCooldown = self.FireRate
    end
end

-- ── Shoot ─────────────────────────────────────────────────────────────────────

function PlayerMovement:Shoot()
    if not self.BulletBlueprint or not self.BulletBlueprint:IsValid() then
        Log.Warn("PlayerMovement: no BulletBlueprint assigned")
        return
    end

    -- Spawn position and fire direction come from BulletPoint (muzzle transform).
    -- Falls back to camera, then player root if neither is set.
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

    -- Spawn bullet and orient it along the fire direction.
    -- PlayerBullet:OnCreate reads GetForward() for its RigidBody velocity.
    local entities = self._scene:Instantiate(self.BulletBlueprint)
    if #entities == 0 then
        Log.Warn("PlayerMovement: Instantiate returned no entities")
        return
    end

    local bullet = entities[1]
    bullet:SetPosition(spawnPos)
    bullet:LookAt(spawnPos + fireDir * 100)
end
