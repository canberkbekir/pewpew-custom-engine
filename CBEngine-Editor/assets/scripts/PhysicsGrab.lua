-- PhysicsGrab.lua
-- Gravity-gun-style grab mechanic: raycast from camera, E to pick up dynamic
-- objects, hold in front of camera, E to drop, left-click to throw.
--
-- Attach to the player entity alongside PlayerMovement.

-- Shared state readable by other scripts (keyed by entity UUID)
PhysicsGrabState = PhysicsGrabState or {}

PhysicsGrab = {
    __fields = {
        GrabRange      = Float(5.0,  0.5, 20.0),
        ThrowForce     = Float(15.0, 1.0, 100.0),
        MoveSmoothing  = Float(15.0, 1.0, 50.0),
        Camera         = EntityRef(),
        GrabPoint      = EntityRef(),   -- transform where held object is positioned (e.g. firepoint)
    }
}

function PhysicsGrab:OnCreate()
    self._grabbed         = nil
    self._grabbedRB       = nil
    self._originalBodyType = nil
    self._originalGravity  = nil
end

function PhysicsGrab:IsHolding()
    return self._grabbed ~= nil
end

function PhysicsGrab:OnUpdate(dt)
    local scene = self._scene

    -- Resolve camera
    local cam = nil
    if self.Camera and self.Camera:IsValid() then
        cam = self.Camera
    end
    if not cam then return end

    local camPos = cam:GetWorldPosition()
    local camFwd = cam:GetForward()

    if not self._grabbed then
        -- Not holding: check for grab on E press
        if Input.IsKeyJustPressed(Key.E) then
            local hit = Physic.Raycast(scene, camPos, camFwd, self.GrabRange, Layer.All, self._entity)
            if hit then
                local hitEntity = hit:GetEntity(scene)
                if hitEntity and hitEntity:IsValid() then
                    local rb = hitEntity:GetComponent(RigidBody)
                    local joint = hitEntity:GetComponent(HingeJoint)
                    if rb and rb:GetBodyType() == "dynamic" and not joint then
                        self:_Grab(hitEntity, rb)
                    end
                end
            end
        end
    else
        -- Holding an object
        if not self._grabbed:IsValid() then
            self:_ClearState()
            return
        end

        if Input.IsKeyJustPressed(Key.E) then
            -- Drop
            self:_Release(Vector3(0, 0, 0))
        elseif Input.IsKeyJustPressed(Key.MouseLeft) then
            -- Throw along camera forward
            local throwDir = cam:GetForward()
            self:_Release(throwDir * self.ThrowForce)
        else
            -- Move held object toward grab point
            local grabPt = self.GrabPoint
            if not grabPt or not grabPt:IsValid() then
                self:_Release(Vector3(0, 0, 0))
                return
            end
            local targetPos = grabPt:GetWorldPosition()
            local currentPos = self._grabbed:GetWorldPosition()
            local alpha = 1.0 - math.exp(-self.MoveSmoothing * dt)
            local newPos = currentPos + (targetPos - currentPos) * alpha
            self._grabbed:SetPosition(newPos)

            -- Zero angular velocity to prevent spinning
            self._grabbedRB:SetAngularVelocity(Vector3(0, 0, 0))
            -- Zero linear velocity so kinematic body doesn't drift
            self._grabbedRB:SetLinearVelocity(Vector3(0, 0, 0))
        end
    end
end

function PhysicsGrab:_Grab(entity, rb)
    self._grabbed          = entity
    self._grabbedRB        = rb
    self._originalBodyType = rb:GetBodyType()
    self._originalGravity  = rb:IsUsingGravity()

    rb:SetBodyType("kinematic")
    rb:SetLinearVelocity(Vector3(0, 0, 0))
    rb:SetAngularVelocity(Vector3(0, 0, 0))

    PhysicsGrabState[self._entity:GetUUID()] = true
end

function PhysicsGrab:_Release(velocity)
    if not self._grabbed or not self._grabbed:IsValid() then
        self:_ClearState()
        return
    end

    self._grabbedRB:SetBodyType(self._originalBodyType or "dynamic")
    self._grabbedRB:SetUseGravity(self._originalGravity ~= false)
    self._grabbedRB:SetLinearVelocity(velocity)
    self._grabbedRB:SetAngularVelocity(Vector3(0, 0, 0))

    self:_ClearState()
end

function PhysicsGrab:_ClearState()
    self._grabbed          = nil
    self._grabbedRB        = nil
    self._originalBodyType = nil
    self._originalGravity  = nil

    PhysicsGrabState[self._entity:GetUUID()] = false
end

function PhysicsGrab:OnDestroy()
    -- Drop on script destroy so objects don't stay kinematic
    if self._grabbed and self._grabbed:IsValid() then
        self:_Release(Vector3(0, 0, 0))
    end
end
