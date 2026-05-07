-- WinPlate.lua
-- Trigger volume: when the player steps on it the game closes (you win).
--
-- Entity setup:
--   Collider with IsTrigger = true
--   ScriptComponent → this script
--   (No RigidBody needed — static trigger is fine)

WinPlate = {
    __fields = {
        Delay = Float(1.0, 0.0, 10.0),  -- seconds to wait before closing (so Win log is visible)
    }
}

function WinPlate:OnCreate()
    self._triggered = false
    local me = self
    me.Collision.OnTriggerEnter:Connect(function(other, point, normal)
        if me._triggered then return end
        if other:GetLayer() ~= Layer.Player then return end
        me._triggered = true
        Log.Info("=== YOU WIN! ===")
        if me.Delay > 0 then
            Tween.Delay(me.Delay, function()
                Application_Quit()
            end)
        else
            Application_Quit()
        end
    end)
end
