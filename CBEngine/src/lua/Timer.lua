-- Timer utility — callback-based countdown/interval helper.
-- This file is documentation only; the Timer global is registered by the engine.
--
-- Usage:
--   self.shootTimer = Timer.New(0.5, function() self:Shoot() end, true)
--   -- in OnUpdate:
--   self.shootTimer:Update(dt)

---@class Timer
---@field _duration number
---@field _time number
---@field _loop boolean
---@field _done boolean
---@field _active boolean
local Timer = {}

--- Create a new Timer.
---@param duration number Seconds until the callback fires (and repeats if loop=true)
---@param callback fun() Function called when the timer expires
---@param loop? boolean If true, the timer restarts automatically. Default: false.
---@return Timer
function Timer.New(duration, callback, loop) end

--- Advance the timer by dt seconds. Call this in OnUpdate.
---@param dt number Delta time in seconds
function Timer:Update(dt) end

--- Reset the timer back to zero and reactivate it.
function Timer:Reset() end

--- Stop the timer without resetting elapsed time.
function Timer:Stop() end

--- Resume a stopped timer.
function Timer:Start() end

--- Returns true if the timer has fired and is not looping.
---@return boolean
function Timer:IsDone() end

--- Returns true if the timer is running.
---@return boolean
function Timer:IsActive() end

--- Returns elapsed time in seconds.
---@return number
function Timer:GetTime() end

--- Returns the timer's duration in seconds.
---@return number
function Timer:GetDuration() end

--- Returns elapsed / duration clamped to [0, 1].
---@return number
function Timer:GetProgress() end
