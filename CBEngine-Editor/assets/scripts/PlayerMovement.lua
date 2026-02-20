-- PlayerMovement.lua
-- Simple physics-based player using RigidBody component directly
-- Uses entity:GetComponent(RigidBody) to access force/impulse methods

PlayerMovement = {
    __fields = {
        MoveSpeed   = Float(12.0, 0, 100),
        JumpForce   = Float(7.0, 0, 50),
        MaxSpeed    = Float(8.0, 0, 50),
        GroundDrag  = Float(6.0, 0, 20),
        AirControl  = Float(0.3, 0, 1),
        GroundCheckDist = Float(1.2, 0, 5),
    }
}

function PlayerMovement:OnCreate()
    local entity = self._entity

    self.isGrounded = false
    self.canJump = false

    local rb = entity:GetComponent(RigidBody)
    if not rb then
        Log.Error("PlayerMovement: Entity '" .. entity:GetName() .. "' needs a RigidBody!")
        return
    end

    -- Lock angular rotation so player doesn't topple over
    rb:SetAngularDamping(100.0)

    Log.Info("PlayerMovement attached to: " .. entity:GetName())
end

function PlayerMovement:OnUpdate(dt) 
    local entity = self._entity
    local scene  = self._scene

    local rb = entity:GetComponent(RigidBody)
    if not rb then return end

    local pos = entity:GetWorldPosition()
    local vel = rb:GetLinearVelocity()

    -- Get camera directions for camera-relative movement
    local camEntity = scene:GetMainCamera()
    local camForward = camEntity and camEntity:GetForward() or Vec3(0, 0, 1)
    local camRight   = camEntity and camEntity:GetRight()   or Vec3(1, 0, 0)

    -- Flatten to XZ plane so we don't move into the ground
    camForward = Vec3(camForward.x, 0, camForward.z)
    if camForward:Length() > 0.001 then camForward = camForward:Normalized() end
    camRight = Vec3(camRight.x, 0, camRight.z)
    if camRight:Length() > 0.001 then camRight = camRight:Normalized() end

    -- Ground check via raycast
    local rayStart = pos + Vec3(0, 0.25, 0)
    local hit = scene:Raycast(rayStart, Vec3(0, -1, 0), self.GroundCheckDist, Layer.All, entity)
    self.isGrounded = hit ~= nil and hit.normal.y > 0.5

    -- Movement input (camera-relative: W = camera forward, D = camera right, etc.)
    local moveDir = Vec3(0, 0, 0)
    if Input.IsKeyPressed(Key.W) then moveDir = moveDir + camForward end
    if Input.IsKeyPressed(Key.S) then moveDir = moveDir - camForward end
    if Input.IsKeyPressed(Key.A) then moveDir = moveDir - camRight   end
    if Input.IsKeyPressed(Key.D) then moveDir = moveDir + camRight   end

    -- Flatten & normalize
    moveDir = Vec3(moveDir.x, 0, moveDir.z)
    local hasInput = moveDir:Length() > 0.01
    if hasInput then
        moveDir = moveDir:Normalized()
    end

    -- Apply movement force (reduced in air)
    local mult = self.isGrounded and 1.0 or self.AirControl
    if hasInput then
        rb:AddForce(moveDir * self.MoveSpeed * mult)
    end

    -- Speed clamping on XZ
    local horizVel = Vec3(vel.x, 0, vel.z)
    local speed = horizVel:Length()
    if speed > self.MaxSpeed then
        local brake = horizVel:Normalized() * (-(speed - self.MaxSpeed) * self.GroundDrag)
        rb:AddForce(brake)
    end

    -- Drag when no input
    if not hasInput and speed > 0.1 and self.isGrounded then
        rb:AddForce(horizVel * -self.GroundDrag)
    end

    -- Jump
    if Input.IsKeyPressed(Key.Space) and self.isGrounded and self.canJump then
        rb:AddImpulse(Vec3(0, self.JumpForce, 0))
        self.canJump = false
        self.isGrounded = false
    end

    -- Rotation with Q/E
    if Input.IsKeyPressed(Key.Q) then
        local rot = entity:GetRotation()
        entity:SetRotation(Vec3(rot.x, rot.y + 3.0 * dt, rot.z))
    end
    if Input.IsKeyPressed(Key.E) then
        local rot = entity:GetRotation()
        entity:SetRotation(Vec3(rot.x, rot.y - 3.0 * dt, rot.z))
    end
end

function PlayerMovement:OnCollisionBegin(other, contactPoint, contactNormal)
    if contactNormal and contactNormal.y > 0.5 then
        self.isGrounded = true
        self.canJump = true
    end
end

function PlayerMovement:OnDestroy()
    Log.Info("PlayerMovement removed from: " .. self._entity:GetName())
end
