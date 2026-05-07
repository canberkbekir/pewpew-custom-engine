-- BurnActivate.lua
-- Entity starts as a Kinematic RigidBody (immovable).
-- Once accumulated heat nearby exceeds HeatThreshold the body
-- switches to Dynamic and falls/can be pushed.
--
-- Setup:
--   1. Add RigidBody to the entity, set Type = Kinematic
--   2. Add this ScriptComponent
--   3. Tune HeatThreshold and CheckRadius in the editor

BurnActivate = {
    __fields = {
        HeatThreshold  = Float(300.0, 1.0, 10000.0), -- total heat in radius to trigger
        CheckRadius    = Float(3.0,   0.5, 20.0),    -- sampling sphere around entity center
        ActivateImpulse = Vec3(0, 2, 0),             -- impulse applied the moment it activates
    }
}

function BurnActivate:OnCreate()
    self._activated = false
    self._rb = self._entity:GetComponent(RigidBody)
    if not self._rb then
        Log.Warn("BurnActivate: entity needs a RigidBody component (set to Kinematic)")
    end
end

function BurnActivate:OnUpdate(dt)
    if self._activated then return end
    if not self._rb   then return end

    -- Sum all heat nodes inside CheckRadius
    local pos   = self._entity:GetWorldPosition()
    local nodes = Heat.QuerySphere(self._scene, pos, self.CheckRadius)
    local total = 0.0
    for _, node in ipairs(nodes) do
        total = total + node.Heat
    end

    if total >= self.HeatThreshold then
        self._activated = true
        self._rb:SetBodyType("dynamic")

        -- Small initial impulse so the wall starts moving immediately
        local imp = self.ActivateImpulse
        if imp:Length() > 0.001 then
            self._rb:AddImpulse(imp)
        end

        Log.Info("BurnActivate: wall is now free!")
    end
end
