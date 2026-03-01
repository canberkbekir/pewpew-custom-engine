-- Interactable.lua
-- Attach this to ANY object you want to be interactable.
-- Other scripts on the same entity connect to onInteract in their OnCreate.
--
-- Usage (on same entity):
--   function MyDoor:OnCreate()
--       local interactable = self._entity:GetScript(Interactable)
--       interactable.onInteract:Connect(function(interactor)
--           self:Toggle()
--       end)
--   end

Interactable = {
    __fields = {
        Prompt          = String("Press [E] to interact"),
        OneTime         = Bool(false),
        Enabled         = Bool(true),
    }
}

function Interactable:OnCreate()
    self.onInteract    = Signal.New()   -- fires when player interacts
    self.onFocusEnter  = Signal.New()   -- fires when player aims at this
    self.onFocusExit   = Signal.New()   -- fires when player looks away
    self._used         = false
    self._focused      = false
end

-- Called by PlayerInteraction when interaction key is pressed.
function Interactable:Interact(interactor)
    if not self.Enabled then return end
    if self.OneTime and self._used then return end
    self._used = true
    self.onInteract:Fire(interactor, self._entity)
end

-- Called by PlayerInteraction every frame to drive focus state.
function Interactable:_SetFocused(focused, interactor)
    if focused == self._focused then return end
    self._focused = focused
    if focused then
        self.onFocusEnter:Fire(interactor, self._entity)
    else
        self.onFocusExit:Fire(interactor, self._entity)
    end
end

-- Returns whether this object can currently be interacted with.
function Interactable:CanInteract()
    if not self.Enabled then return false end
    if self.OneTime and self._used then return false end
    return true
end

-- Reset a OneTime interactable back to unused.
function Interactable:Reset()
    self._used = false
end
