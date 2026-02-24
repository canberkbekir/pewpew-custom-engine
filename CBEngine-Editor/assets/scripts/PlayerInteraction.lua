-- PlayerInteraction.lua
-- Attach to the player entity alongside PlayerMovement.
-- Raycasts from camera every frame; on [E] press, interacts with
-- the focused Interactable.
--
-- Requires on entity:
--   - Camera child (or CameraPoint child with Camera sub-child)
--
-- Optional: wire onInteractFocus / onInteractLost signals externally
-- to drive UI prompts.

PlayerInteraction = {
    __fields = {
        InteractRange       = Float(3.0, 0.1, 50.0),
        InteractLayerMask   = Int(65535),   -- Layer.All by default; narrow if needed
        CameraEntityName    = String("Camera"),
    }
}

function PlayerInteraction:OnCreate()
    self._cameraEntity   = nil
    self._focused        = nil   -- current Interactable script being looked at
    self.onFocusEnter    = Signal.New()   -- fired with (interactable, entity) when aim begins
    self.onFocusExit     = Signal.New()   -- fired with (interactable, entity) when aim ends
    self:_FindCamera()
end

function PlayerInteraction:_FindCamera()
    -- Walk children looking for the camera entity by name.
    local children = self._entity:GetChildren()
    for _, child in ipairs(children) do
        if child:GetName() == self.CameraEntityName then
            self._cameraEntity = child
            return
        end
        -- One level deeper (CameraPoint -> Camera)
        local grandchildren = child:GetChildren()
        for _, gc in ipairs(grandchildren) do
            if gc:GetName() == self.CameraEntityName then
                self._cameraEntity = gc
                return
            end
        end
    end
    if not self._cameraEntity then
        Log.Warn("[PlayerInteraction] Camera entity '" .. self.CameraEntityName .. "' not found as child.")
    end
end

function PlayerInteraction:OnUpdate(dt)
    if not self._cameraEntity or not self._cameraEntity:IsValid() then
        self:_FindCamera()
        return
    end

    local origin  = self._cameraEntity:GetWorldPosition()
    local forward = self._cameraEntity:GetForward()

    -- Raycast ignoring the player entity itself.
    local hit = Physic.Raycast(
        self._scene,
        origin,
        forward,
        self.InteractRange,
        self.InteractLayerMask,
        self._entity
    )

    local hitInteractable = nil

    if hit then
        local hitEntity = hit:GetEntity(self._scene)
        if hitEntity and hitEntity:IsValid() then
            local interactable = hitEntity:GetScript(Interactable)
            if interactable and interactable:CanInteract() then
                hitInteractable = interactable
            end
        end
    end

    -- Drive focus state changes.
    if hitInteractable ~= self._focused then
        -- Lost previous focus.
        if self._focused then
            self._focused:_SetFocused(false, self._entity)
            self.onFocusExit:Fire(self._focused, self._focused._entity)
        end
        -- Gained new focus.
        self._focused = hitInteractable
        if self._focused then
            self._focused:_SetFocused(true, self._entity)
            self.onFocusEnter:Fire(self._focused, self._focused._entity)
        end
    end

    -- Trigger interaction on key press.
    if self._focused and Input.IsKeyJustPressed(Key.E) then
        self._focused:Interact(self._entity)
    end
end

function PlayerInteraction:OnDestroy()
    if self._focused then
        self._focused:_SetFocused(false, self._entity)
        self._focused = nil
    end
end

-- Returns the currently focused Interactable script, or nil.
function PlayerInteraction:GetFocused()
    return self._focused
end
