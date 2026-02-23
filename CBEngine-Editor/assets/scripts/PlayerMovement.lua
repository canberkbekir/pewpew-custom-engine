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

        -- Look
        MouseSensitivity  = Float(0.002, 0, 0.02),
        PitchSensitivity  = Float(0.002, 0, 0.02),
        PitchMin         = Float(-85.0, -89, 0),
        PitchMax         = Float(85.0,   0, 89),
        LookSmoothTime   = Float(0.05, 0.0, 0.3),

        -- Shooting
        BulletBlueprint  = BlueprintRef(),   -- drag .blueprint or scene entity here
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

    -- Look state
    local rot = entity:GetRotation()
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
    self.rb:SetAngularDamping(100.0)

    -- Resolve camera entity
    if self.Camera and self.Camera:IsValid() then
        self.cam = self.Camera
    else
        for _, child in ipairs(entity:GetChildren()) do
            if child:HasCamera() then
                self.cam = child
                break
            end
        end
    end

    -- Resolve pitch pivot (intermediate parent between player and camera)
    if self.cam and self.cam:IsValid() then
        if self.cam:HasParent() then
            local p = self.cam:GetParent()
            if p and p:IsValid() and p:GetUUID() ~= entity:GetUUID() then
                self.pitchNode = p
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

    -- ── Look ──────────────────────────────────────────────────────────────────
    local md = Input.GetMouseDelta()
    self.yawTarget   = self.yawTarget   - md.x * self.MouseSensitivity
    self.pitchTarget = self.pitchTarget + md.y * self.PitchSensitivity
    self.pitchTarget = Math.Clamp(self.pitchTarget,
        Math.Rad(self.PitchMin), Math.Rad(self.PitchMax))

    local a = ExpAlpha(dt, self.LookSmoothTime)
    self.yawSmoothed   = self.yawSmoothed   + (self.yawTarget   - self.yawSmoothed)   * a
    self.pitchSmoothed = self.pitchSmoothed + (self.pitchTarget - self.pitchSmoothed) * a

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
    local move = Vector3(0, 0, 0)
    if Input.IsKeyPressed(Key.W) then move = move + Vector3(0, 0, 1) end
    if Input.IsKeyPressed(Key.S) then move = move - Vector3(0, 0, 1) end
    if Input.IsKeyPressed(Key.A) then move = move - Vector3(1, 0, 0) end
    if Input.IsKeyPressed(Key.D) then move = move + Vector3(1, 0, 0) end

    move = FlattenXZ(move)
    local hasMove = move:Length() > 0.01
    move = SafeNormalize(move)

    local forward, right = YawBasis(self.yawSmoothed)
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

    -- Muzzle position and aim direction come from the camera (FPS accuracy).
    -- If no camera is set, fall back to the player entity.
    local muzzlePos, aimDir
    if self.cam and self.cam:IsValid() then
        muzzlePos = self.cam:GetWorldPosition()
        aimDir    = self.cam:GetForward()
    else
        muzzlePos = self._entity:GetWorldPosition()
        aimDir    = self._entity:GetForward()
    end

    -- Raycast from camera forward to find the precise hit point.
    -- This ensures the bullet travels toward whatever the crosshair is over,
    -- even when the muzzle is slightly offset from the camera.
    local rayHit = Physic.Raycast(
        self._scene, muzzlePos, aimDir, 1000,
        Layer.Mask(Layer.Default, Layer.Enemy, Layer.Environment),
        self._entity
    )

    local targetPoint
    if rayHit then
        targetPoint = rayHit.point
    else
        targetPoint = muzzlePos + aimDir * 1000
    end

    -- Spawn bullet hierarchy with fresh UUIDs.
    local entities = self._scene:InstantiateBlueprint(self.BulletBlueprint)
    if #entities == 0 then
        Log.Warn("PlayerMovement: InstantiateBlueprint returned no entities")
        return
    end

    local bullet = entities[1]

    -- Place bullet at the camera/muzzle, oriented toward the target point.
    -- Bullet:OnCreate fires next frame and reads GetForward() for velocity.
    bullet:SetPosition(muzzlePos)
    bullet:LookAt(targetPoint)
end
