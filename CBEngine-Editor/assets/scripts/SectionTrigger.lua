SectionTrigger = {
    __fields = {
        SectionId = Int(0, 0, 99),
    }
}

function SectionTrigger:OnCreate()
    self._triggered = false
    local me = self
    me.Collision.OnTriggerEnter:Connect(function(other, point, normal)
        if me._triggered then return end
        if other:GetLayer() ~= Layer.Player then
            local rb = other:GetComponent(RigidBody)
            if not rb or rb:GetBodyType() ~= "dynamic" then return end
        end
        me._triggered = true
        local gm = me._scene:GetGameManager()
        if gm and gm:IsValid() then
            local lm = gm:GetScript(LevelManager)
            if lm then
                if me.SectionId >= 99 then
                    lm:OnLevelComplete()
                else
                    lm:OnSectionEntered(me.SectionId)
                end
            end
        end
    end)
end

function SectionTrigger:OnDestroy()
end
