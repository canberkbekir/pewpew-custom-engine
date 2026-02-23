-- InfinityFlight.lua
-- Moves an entity along a figure-8 / infinity symbol path in the XZ plane.
-- Uses the lemniscate parametric equations:
--   x(t) = Width  * sin(t)
--   z(t) = Depth  * sin(2t)

InfinityFlight = {
    __fields = {
        Width    = Float(10.0, 0.5, 200.0),   -- half-width of the figure-8
        Depth    = Float(5.0,  0.5, 100.0),   -- half-depth of each loop
        Speed    = Float(1.0,  0.1, 10.0),    -- angular speed (radians/sec)
        Altitude = Float(0.0, -500.0, 500.0), -- height offset from spawn position
        MaxBank  = Float(40.0, 0.0, 89.0),    -- max roll angle in degrees
    }
}
InfinityFlight.__index = InfinityFlight

function InfinityFlight:OnCreate()
    self._t      = 0.0
    self._origin = self._entity:GetPosition()
end

function InfinityFlight:OnUpdate(dt)
    self._t = self._t + dt * self.Speed

    local t = self._t
    local W = self.Width
    local D = self.Depth

    -- Position along the lemniscate
    local px = W * Math.Sin(t)
    local pz = D * Math.Sin(2 * t)

    self._entity:SetPosition(Vector3(
        self._origin.x + px,
        self._origin.y + self.Altitude,
        self._origin.z + pz
    ))

    -- First derivative: velocity direction
    --   dx/dt = W  * cos(t)
    --   dz/dt = 2D * cos(2t)
    local vx = W * Math.Cos(t)
    local vz = 2 * D * Math.Cos(2 * t)
    local speed = math.sqrt(vx * vx + vz * vz)

    if speed > 0.001 then
        -- Yaw: face the direction of travel
        local yaw = Math.Atan2(vx, vz)

        -- Second derivative: centripetal acceleration
        --   ax/dt = -W  * sin(t)
        --   az/dt = -4D * sin(2t)
        local ax = -W * Math.Sin(t)
        local az = -4 * D * Math.Sin(2 * t)

        -- Right vector in XZ (perpendicular to forward, 90° clockwise)
        local rx =  vz / speed
        local rz = -vx / speed

        -- Lateral (centripetal) acceleration → bank angle
        local latAccel  = ax * rx + az * rz
        local curvature = latAccel / (speed * speed)
        local bank = Math.Clamp(curvature * 3.0, -1.0, 1.0) * math.rad(self.MaxBank)

        self._entity:SetRotation(Vector3(0, yaw, bank))
    end
end
