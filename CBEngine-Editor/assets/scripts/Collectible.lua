-- Collectible.lua
-- Attach to entities with a trigger collider. When the player touches it,
-- notifies the CollectGameManager and hides/destroys this entity.

Collectible = {
    __fields = {
        PointValue   = Int(1, 1, 100),
        BobSpeed     = Float(2.0, 0, 10),
        BobHeight    = Float(0.3, 0, 2),
        SpinSpeed    = Float(2.0, 0, 10),
        DestroyOnCollect = Bool(true),
    }
}

function Collectible:OnCreate()
    self.collected = false
    self.startPos  = self._entity:GetPosition()
    self.time      = 0.0

    Log.Info("Collectible spawned: " .. self._entity:GetName()
        .. " (worth " .. self.PointValue .. " pts)")
end

function Collectible:OnUpdate(dt)
    if self.collected then return end

    self.time = self.time + dt

    -- Floating bob animation
    local newY = self.startPos.y + math.sin(self.time * self.BobSpeed) * self.BobHeight
    self._entity:SetPosition(Vec3(self.startPos.x, newY, self.startPos.z))

    -- Spin rotation
    local rot = self._entity:GetRotation()
    self._entity:SetRotation(Vec3(rot.x, rot.y + self.SpinSpeed * dt, rot.z))
end

function Collectible:OnCollisionBegin(other, contactPoint, contactNormal)
    if self.collected then return end

    -- Only respond to Player layer entities (or any entity with a RigidBody that is dynamic)
    if other:GetLayer() ~= Layer.Player then
        -- Fallback: check if other has a PlayerMovement script
        -- (in case layer isn't set, still allow collection from dynamic bodies)
        local rb = other:GetComponent(RigidBody)
        if not rb or rb:GetBodyType() ~= "dynamic" then
            return
        end
    end

    self.collected = true
    Log.Info(other:GetName() .. " collected " .. self._entity:GetName()
        .. " (+" .. self.PointValue .. ")")

    -- Notify game manager
    local gm = self._scene:GetGameManager()
    if gm and gm:IsValid() then
        local gmScript = gm:GetScript(CollectGameManager)
        if gmScript then
            gmScript:OnItemCollected(self.PointValue, self._entity)
        end
    end

    -- Hide or destroy
    if self.DestroyOnCollect then
        self._scene:DestroyEntity(self._entity)
    else
        self._entity:SetVisible(false)
    end
end

function Collectible:OnDestroy()
end
