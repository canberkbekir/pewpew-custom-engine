-- CustomGameManager.lua
-- Inherits from GameManager base class

CustomGameManager = GameManager:Extend()

CustomGameManager.__fields = {
    -- MaxScore = Int(100),
    -- GameTime = Float(120.0, 0, 600),
}

function CustomGameManager:OnCreate()
    -- Called when the game manager is created
    Log.Info("Test")
end

function CustomGameManager:OnUpdate(dt)
    -- Called every frame
end

function CustomGameManager:OnDestroy()
    -- Called when the game manager is destroyed
end
