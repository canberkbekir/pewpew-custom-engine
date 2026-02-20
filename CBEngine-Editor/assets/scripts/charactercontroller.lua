-- CharacterController.lua
-- Physics-based character controller with improved ground detection + extra logging

CharacterController = {
    __fields = {
        MoveSpeed        = Float(15.0, 0, 100),
        JumpForce        = Float(8.0, 0, 50),
        MaxSpeed         = Float(10.0, 0, 50),
        GroundDrag       = Float(5.0, 0, 20),
        AirDrag          = Float(0.5, 0, 10),
        RotationSpeed    = Float(3.0, 0, 10),

        GroundCheckDist  = Float(1.2, 0, 5),
        GroundCheckRadius= Float(0.25, 0, 2),   -- used for multi-ray spread
        GroundSnapEps    = Float(0.08, 0, 1),    -- small tolerance for slopes/steps
        GroundRayStartUp = Float(0.25, 0, 2),    -- start ray a bit above feet
        DebugLogs        = Bool(true),
        Test             = Vector3(0),
    }
}

-- ------------------------------------------------------------
-- Small helpers
-- ------------------------------------------------------------
local function _KeyDown()
    return Input.IsKeyPressed(Key.W) or Input.IsKeyPressed(Key.A) or Input.IsKeyPressed(Key.S) or Input.IsKeyPressed(Key.D)
end

local function _Log(self, msg)
    if self.DebugLogs then
        Log.Info(msg)
    end
end

-- Safely read common raycast result shapes.
-- Your engine might return:
--  1) nil / false for no hit
--  2) table with fields: point, normal, distance, entityId...
--  3) userdata with .point .normal .distance
local function _HitPoint(hit)
    return hit and hit.point
end
local function _HitNormal(hit)
    return hit and hit.normal
end
local function _HitDistance(hit)
    return hit and hit.distance
end

-- ------------------------------------------------------------
-- Lifecycle
-- ------------------------------------------------------------
function CharacterController:OnCreate()
    local entity = self._entity

    self.isGrounded = false
    self.canJump = false
    self._wasGrounded = false

    -- throttle for per-second logs
    self._logAcc = 0.0
    self._lastGroundLog = 0.0

    if not entity:HasRigidBody() then
        Log.Error("CharacterController: Entity needs a RigidBody component!")
        return
    end

    entity:SetAngularDamping(10.0)

    _Log(self,
        "CharacterController attached to: " .. entity:GetName()
        .. " (Layer: " .. entity:GetLayer() .. ")"
    )
    _Log(self,
        string.format("GroundCheckDist=%.2f GroundRayStartUp=%.2f GroundSnapEps=%.2f GroundCheckRadius=%.2f",
            self.GroundCheckDist, self.GroundRayStartUp, self.GroundSnapEps, self.GroundCheckRadius
        )
    )
end

-- Ground test:
-- - ray origin is slightly above the feet (avoids starting inside ground/collider)
-- - use several rays (center + 4 offsets) to handle steps/edges and uneven ground
-- - apply a small epsilon so tiny gaps don’t flicker grounded state
function CharacterController:_UpdateGrounded(scene, pos, rayMask)
    local down = Vec3(0, -1, 0)

    local start = pos + Vec3(0, self.GroundRayStartUp, 0)
    local dist = self.GroundCheckDist + self.GroundSnapEps

    local r = self.GroundCheckRadius
    local offsets = {
        Vec3(0, 0, 0),
        Vec3( r, 0, 0),
        Vec3(-r, 0, 0),
        Vec3(0, 0,  r),
        Vec3(0, 0, -r),
    }

    local bestHit = nil
    local bestDist = 1e9

    for i = 1, #offsets do
        local o = start + offsets[i]
        local hit = scene:Raycast(o, down, dist, rayMask, self._entity)

        -- draw each ray (slightly different colors)
        if self.DebugLogs then
            local c = Vec3(0.6, 0.0, 0.6)
            Debug.DrawRay(o, down, dist, c)
        end

        if hit then
            local d = _HitDistance(hit) or dist
            if d < bestDist then
                bestDist = d
                bestHit = hit
            end
        end
    end

    if bestHit then
        local n = _HitNormal(bestHit)
        local d = bestDist

        -- basic slope check: treat as ground only if normal points up enough
        -- (0.5 ~= ~60 degrees max slope). Adjust if needed.
        local slopeOk = n and n.y and n.y > 0.5

        if slopeOk and d <= dist then
            self.isGrounded = true
            self.canJump = true

            -- draw hit normal
            local p = _HitPoint(bestHit)
            if p and n then
                Debug.DrawLine(p, p + n * 0.3, Vec3(0, 1, 0))
            end

            self._lastGroundHit = bestHit
            self._lastGroundDist = d
            self._lastGroundNormal = n
            return
        end
    end

    self.isGrounded = false
end

function CharacterController:OnUpdate(dt)
    local entity = self._entity
    local scene = self._scene

    if not entity:HasRigidBody() then
        return
    end

    self._logAcc = self._logAcc + dt

    local velocity = scene:GetLinearVelocity(entity)
    local pos = entity:GetWorldPosition()
    local forward = entity:GetForward()
    local right = entity:GetRight()

    -- Ground check (skip IgnoreRaycast layer)
    local rayMask = Layer.All - Layer.Mask(Layer.IgnoreRaycast)
    self:_UpdateGrounded(scene, pos, rayMask)

    -- log only when grounded state changes (plus an occasional status)
    if self.isGrounded ~= self._wasGrounded then
        if self.isGrounded then
            _Log(self, string.format("[Ground] ENTER grounded (dist=%.3f, normalY=%.3f)",
                (self._lastGroundDist or -1),
                (self._lastGroundNormal and self._lastGroundNormal.y or -1)
            ))
        else
            _Log(self, "[Ground] EXIT grounded")
        end
        self._wasGrounded = self.isGrounded
    elseif self.DebugLogs and self._logAcc - self._lastGroundLog > 1.0 then
        self._lastGroundLog = self._logAcc
        _Log(self, string.format("[Status] grounded=%s vel=(%.2f, %.2f, %.2f)",
            tostring(self.isGrounded),
            velocity.x, velocity.y, velocity.z
        ))
    end

    -- Movement input
    local moveDir = Vec3(0, 0, 0)
    if Input.IsKeyPressed(Key.W) then moveDir = moveDir + forward end
    if Input.IsKeyPressed(Key.S) then moveDir = moveDir - forward end
    if Input.IsKeyPressed(Key.A) then moveDir = moveDir - right   end
    if Input.IsKeyPressed(Key.D) then moveDir = moveDir + right   end

    -- Flatten movement to XZ plane and normalize
    moveDir = Vec3(moveDir.x, 0, moveDir.z)
    if moveDir:Length() > 0.01 then
        moveDir = moveDir:Normalized()
        Debug.DrawRay(pos, moveDir, 2.0, Vec3(0, 0.5, 1))
    end

    -- Apply movement force (reduced in air)
    local moveMult = self.isGrounded and 1.0 or 0.3
    local force = moveDir * self.MoveSpeed * moveMult
    scene:AddForce(entity, force)

    -- Speed limiting on XZ plane (brake only when above max speed)
    local horizVel = Vec3(velocity.x, 0, velocity.z)
    local horizontalSpeed = horizVel:Length()
    if horizontalSpeed > self.MaxSpeed then
        local brakeDir = horizVel:Normalized()
        local excess = (horizontalSpeed - self.MaxSpeed)
        local brake = brakeDir * (-excess * self.GroundDrag)
        scene:AddForce(entity, brake)
    end

    -- Apply drag when not pressing movement keys
    local currentDrag = self.isGrounded and self.GroundDrag or self.AirDrag
    if (not _KeyDown()) and horizontalSpeed > 0.1 then
        local drag = horizVel * -currentDrag
        scene:AddForce(entity, drag)
    end

    -- Jump (only when grounded)
    if Input.IsKeyPressed(Key.Space) and self.canJump and self.isGrounded then
        _Log(self, string.format("[Jump] impulse=%.2f", self.JumpForce))
        scene:AddImpulse(entity, Vec3(0, self.JumpForce, 0))
        self.canJump = false
        self.isGrounded = false
        self._wasGrounded = false
    end

    -- Rotation with Q/E
    if Input.IsKeyPressed(Key.Q) then
        local rot = entity:GetRotation()
        entity:SetRotation(Vec3(rot.x, rot.y + self.RotationSpeed * dt, rot.z))
    end
    if Input.IsKeyPressed(Key.E) then
        local rot = entity:GetRotation()
        entity:SetRotation(Vec3(rot.x, rot.y - self.RotationSpeed * dt, rot.z))
    end

    -- Forward raycast to detect walls/objects ahead
    local lookHit = scene:Raycast(pos, forward, 5.0, rayMask, self._entity)
    if lookHit then
        Debug.DrawLine(pos, lookHit.point, Vec3(1, 1, 0))
        Debug.DrawLine(lookHit.point, lookHit.point + lookHit.normal * 0.5, Vec3(0, 1, 1))

        -- Avoid spamming trace every frame: log at most once per second
        if self.DebugLogs and self._logAcc % 1.0 < dt then
            local hitEntity = lookHit:GetEntity(scene)
            if hitEntity and hitEntity:IsValid() then
                _Log(self, "Looking at: " .. hitEntity:GetName()
                    .. " (dist: " .. string.format("%.2f", lookHit.distance) .. ")")
            end
        end
    end
end

function CharacterController:OnCollisionBegin(other, contactPoint, contactNormal)
    -- Collision-based ground detect as fallback (helps when ray misses small steps)
    if contactNormal and contactNormal.y and contactNormal.y > 0.5 then
        if not self.isGrounded then
            _Log(self, string.format("[Ground] COLLISION grounded (normalY=%.3f)", contactNormal.y))
        end
        self.isGrounded = true
        self.canJump = true
        self._wasGrounded = true
    end

    Debug.DrawLine(contactPoint, contactPoint + contactNormal * 0.5, Vec3(1, 0.5, 0), 0.5)
end

function CharacterController:OnCollisionEnd(other)
    -- If you want robust multi-contact grounding, track a "groundContacts" counter here.
end

function CharacterController:OnValidate(fields, changedField)
    -- Ensure MaxSpeed is never greater than MoveSpeed
    if fields.MaxSpeed > fields.MoveSpeed then
        Log.Warn("MaxSpeed (" .. fields.MaxSpeed .. ") clamped to MoveSpeed (" .. fields.MoveSpeed .. ")")
        fields.MaxSpeed = fields.MoveSpeed
        return fields
    end
end

function CharacterController:OnDestroy()
    Log.Info("CharacterController removed from: " .. self._entity:GetName())
end