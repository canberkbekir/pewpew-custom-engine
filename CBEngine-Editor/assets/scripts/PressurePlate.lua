PressurePlate = {
    __fields = {
        TargetDoor   = EntityRef(),
        HeatAmount   = Float(2000, 0, 50000),
        DamageRadius = Float(0.8, 0.1, 5),
        DamageAmount = Float(800, 0, 5000),
    }
}

function PressurePlate:OnCreate()
    self._triggered = false
    local me = self
    me.Collision.OnTriggerEnter:Connect(function(other, point, normal)
        if me._triggered then return end
        if other:GetLayer() == Layer.Player then return end
        local rb = other:GetComponent(RigidBody)
        if not rb or rb:GetBodyType() ~= "dynamic" then return end
        me._triggered = true
        if me.TargetDoor and me.TargetDoor:IsValid() then
            local pos = me.TargetDoor:GetWorldPosition()
            Heat.Inject(me._scene, pos, me.HeatAmount)
            VoxelDamage.ApplySphere(me._scene, pos, me.DamageRadius,
                { type = DamageType.Fire, amount = me.DamageAmount })
        end
    end)
end

function PressurePlate:OnDestroy()
end
