-- DoorController.lua
-- Example interactable: toggles open/closed by rotating the entity.
-- Requires Interactable.lua on the SAME entity.

DoorController = {
    __fields = {
        OpenAngle    = Vec3(0, 90, 0),
        CloseAngle   = Vec3(0, 0, 0),
        AnimDuration = Float(0.5, 0.05, 5.0),
    }
}

function DoorController:OnCreate()
    self._open     = false
    self._animating = false

    local interactable = self._entity:GetScript(Interactable)
    if not interactable then
        Log.Error("[DoorController] Requires Interactable script on same entity.")
        return
    end

    -- Override the prompt dynamically based on state.
    self._interactable = interactable

    interactable.onInteract:Connect(function(interactor, doorEntity)
        self:Toggle()
    end)

    interactable.onFocusEnter:Connect(function(interactor, doorEntity)
        interactable.Prompt = self._open and "Press [E] to close" or "Press [E] to open"
    end)
end

function DoorController:Toggle()
    if self._animating then return end
    self._open = not self._open
    self:_Animate(self._open)

    if self._interactable then
        self._interactable.Prompt = self._open and "Press [E] to close" or "Press [E] to open"
    end
end

function DoorController:_Animate(opening)
    self._animating = true
    local startRot = self._entity:GetLocalRotation()
    local target   = opening
        and Vector3(self.OpenAngle.x,  self.OpenAngle.y,  self.OpenAngle.z)
        or  Vector3(self.CloseAngle.x, self.CloseAngle.y, self.CloseAngle.z)

    Tween.Value(0, 1, self.AnimDuration)
        :SetEase(Easing.CubicInOut)
        :OnUpdate(function(t)
            local rot = Vector3(
                Math.Lerp(startRot.x, target.x, t),
                Math.Lerp(startRot.y, target.y, t),
                Math.Lerp(startRot.z, target.z, t)
            )
            self._entity:SetLocalRotation(rot)
        end)
        :OnComplete(function()
            self._animating = false
        end)
end
