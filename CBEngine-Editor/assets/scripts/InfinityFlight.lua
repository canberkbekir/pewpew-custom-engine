-- InfinityFlight.lua
-- Moves an entity along a figure-8 / infinity symbol path in the XZ plane.
-- Uses the lemniscate parametric equations:
--   x(t) = Width  * sin(t)
--   z(t) = Depth  * sin(2t)
--
-- Tweens used:
--   OnCreate: speed ramps 0 → Speed over 2 s (CubicIn) — entity launches smoothly
--   OnCreate: altitude rises 0 → Altitude over 3 s (SineOut) — entity climbs to cruise height

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
    self._t               = 0.0
    self._origin          = self._entity:GetPosition()
    self._currentSpeed    = 0.0
    self._currentAltitude = 0.0

    -- Ramp speed from 0 → Speed over 2 s with CubicIn (slow launch, then full throttle)
    Tween.Value(0.0, self.Speed, 2.0)
        :SetEase(Easing.CubicIn)
        :OnUpdate(function(v) self._currentSpeed = v end)

    -- Lift from spawn altitude → target Altitude over 3 s with SineOut (smooth climb)
    Tween.Value(0.0, self.Altitude, 3.0)
        :SetEase(Easing.SineOut)
        :OnUpdate(function(v) self._currentAltitude = v end)
end

function InfinityFlight:OnUpdate(dt)
    self._t = self._t + dt * self._currentSpeed

    local t = self._t
    local W = self.Width
    local D = self.Depth

    -- Position along the lemniscate
    local px = W * Math.Sin(t)
    local pz = D * Math.Sin(2 * t)

    self._entity:SetPosition(Vector3(
        self._origin.x + px,
        self._origin.y + self._currentAltitude,
        self._origin.z + pz
    ))

    -- First derivative: velocity direction
    --   dx/dt = W  * cos(t)
    --   dz/dt = 2D * cos(2t)
    local vx = W * Math.Cos(t)
    local vz = 2 * D * Math.Cos(2 * t)
    local speed = math.sqrt(vx * vx + vz * vz)

    if speed > 0.001 then
        -- Yaw: face the direction of travel (convert radians → degrees for SetRotation)
        local yawDeg = Math.Deg(Math.Atan2(vx, vz))

        -- Second derivative: centripetal acceleration
        --   ax/dt = -W  * sin(t)
        --   az/dt = -4D * sin(2t)
        local ax = -W * Math.Sin(t)
        local az = -4 * D * Math.Sin(2 * t)

        -- Right vector in XZ (perpendicular to forward, 90° clockwise)
        local rx =  vz / speed
        local rz = -vx / speed

        -- Lateral (centripetal) acceleration → bank angle in degrees
        local latAccel  = ax * rx + az * rz
        local curvature = latAccel / (speed * speed)
        local bankDeg   = Math.Clamp(curvature * 3.0, -1.0, 1.0) * self.MaxBank

        -- SetRotation expects degrees
        self._entity:SetRotation(Vector3(0, yawDeg, bankDeg))
    end
end
