-- CollectGameManager.lua
-- GameManager that tracks collected items and triggers a win condition.
-- Attach to a GameManager entity in the scene.

CollectGameManager = GameManager:Extend()

CollectGameManager.__fields = {
    WinScore    = Int(5, 1, 100),
    TimeLimit   = Float(0, 0, 600),   -- 0 = no time limit
}

function CollectGameManager:OnCreate()
    self.score       = 0
    self.gameOver    = false
    self.won         = false
    self.elapsed     = 0.0

    Log.Info("=== Collect Game Started! ===")
    Log.Info("Collect " .. self.WinScore .. " items to win!")
    if self.TimeLimit > 0 then
        Log.Info("Time limit: " .. self.TimeLimit .. " seconds")
    end
end

function CollectGameManager:OnUpdate(dt)
    if self.gameOver then return end

    -- Timer
    if self.TimeLimit > 0 then
        self.elapsed = self.elapsed + dt
        local remaining = self.TimeLimit - self.elapsed

        if remaining <= 0 then
            self.gameOver = true
            self.won = false
            Log.Warn("=== TIME'S UP! You collected " .. self.score .. "/" .. self.WinScore .. " ===")
            return
        end
    end
end

-- Called by Collectible scripts when player picks up an item
function CollectGameManager:OnItemCollected(points, collectibleEntity)
    if self.gameOver then return end

    self.score = self.score + points
    Log.Info("Score: " .. self.score .. " / " .. self.WinScore)

    if self.score >= self.WinScore then
        self.gameOver = true
        self.won = true
        Log.Info("=== YOU WIN! All items collected! ===")
        if self.TimeLimit > 0 then
            Log.Info(string.format("Time: %.1f seconds", self.elapsed))
        end
    end
end

function CollectGameManager:OnDestroy()
    Log.Info("=== Collect Game Ended ===")
    Log.Info("Final Score: " .. self.score .. " / " .. self.WinScore)
end
