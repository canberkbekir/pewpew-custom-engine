---@meta

-- =====================================================================
-- TweenHandle — returned by Tween.Value() and Tween.Delay()
-- =====================================================================

---@class TweenHandle
local TweenHandle = {}

--- Set the easing function. Use an Easing.* constant.
---@param fn function Easing function (e.g. Easing.CubicOut)
---@return TweenHandle
function TweenHandle:SetEase(fn) end

--- Called every frame with the current interpolated value.
---@param fn fun(value: number|Vec3|Vec2)
---@return TweenHandle
function TweenHandle:OnUpdate(fn) end

--- Called once when the tween finishes (or the last loop completes).
---@param fn fun()
---@return TweenHandle
function TweenHandle:OnComplete(fn) end

--- Called once on the first frame the tween becomes active (after any delay).
---@param fn fun()
---@return TweenHandle
function TweenHandle:OnStart(fn) end

--- Wait this many seconds before starting.
---@param seconds number
---@return TweenHandle
function TweenHandle:SetDelay(seconds) end

--- Loop the tween. Pass -1 for infinite loops.
--- loopType: "Restart" (default) or "Yoyo" (ping-pong).
---@param count integer  Number of extra loops (-1 = infinite)
---@param loopType? string  "Restart" or "Yoyo"
---@return TweenHandle
function TweenHandle:SetLoops(count, loopType) end

--- Make the `to` value relative to `from` (i.e. add to current value).
---@return TweenHandle
function TweenHandle:SetRelative() end

--- Restart the tween from the beginning and re-add it to the active list.
---@return TweenHandle
function TweenHandle:Restart() end

--- Stop and remove the tween immediately.
function TweenHandle:Cancel() end

--- Pause the tween (keeps it in the active list but stops ticking).
function TweenHandle:Pause() end

--- Resume a paused tween.
function TweenHandle:Resume() end

--- Returns true while the tween is still running.
---@return boolean
function TweenHandle:IsActive() end


-- =====================================================================
-- TweenSequence — returned by Tween.Sequence()
-- =====================================================================

---@class TweenSequence
local TweenSequence = {}

--- Append a tween to run after all previous tweens finish.
---@param tween TweenHandle
---@return TweenSequence
function TweenSequence:Append(tween) end

--- Run a tween simultaneously with the last Append.
---@param tween TweenHandle
---@return TweenSequence
function TweenSequence:Join(tween) end

--- Insert an empty gap of `seconds` after the current head.
---@param seconds number
---@return TweenSequence
function TweenSequence:AppendInterval(seconds) end

--- Wait this many seconds before the sequence starts.
---@param seconds number
---@return TweenSequence
function TweenSequence:SetDelay(seconds) end

--- Loop the sequence. Pass -1 for infinite.
---@param count integer
---@param loopType? string  "Restart" or "Yoyo"
---@return TweenSequence
function TweenSequence:SetLoops(count, loopType) end

--- Called once when the whole sequence finishes.
---@param fn fun()
---@return TweenSequence
function TweenSequence:OnComplete(fn) end

--- Called once when the sequence first becomes active.
---@param fn fun()
---@return TweenSequence
function TweenSequence:OnStart(fn) end

--- Stop the sequence immediately.
function TweenSequence:Cancel() end

--- Pause the sequence.
function TweenSequence:Pause() end

--- Resume a paused sequence.
function TweenSequence:Resume() end

--- Returns true while the sequence is still running.
---@return boolean
function TweenSequence:IsActive() end


-- =====================================================================
-- Tween — global entry points
-- =====================================================================

---@class Tween
Tween = {}

--- Interpolate a number, Vec3, or Vec2 from `from` to `to` over `duration` seconds.
--- Chain fluent methods to configure: :SetEase(), :OnUpdate(), :OnComplete(), etc.
---
--- Example:
---   Tween.Value(0, 10, 2.0)
---       :SetEase(Easing.CubicOut)
---       :OnUpdate(function(v) self._entity:SetPosition(Vector3(v, 0, 0)) end)
---
---@param from number|Vec3|Vec2
---@param to   number|Vec3|Vec2
---@param duration number Seconds
---@return TweenHandle
function Tween.Value(from, to, duration) end

--- Fire `fn` after `seconds` with no value interpolation.
--- Shorthand for Tween.Value(0,1,seconds):OnComplete(fn).
---
--- Example:
---   Tween.Delay(3.0, function() self._entity:SetVisible(false) end)
---
---@param seconds number
---@param fn fun()
---@return TweenHandle
function Tween.Delay(seconds, fn) end

--- Create a new sequence for chaining/grouping tweens.
---
--- Example:
---   Tween.Sequence()
---       :Append(Tween.Value(0, 5, 1.0):SetEase(Easing.SineOut):OnUpdate(fn))
---       :AppendInterval(0.5)
---       :Append(Tween.Value(5, 0, 1.0):SetEase(Easing.SineIn):OnUpdate(fn))
---
---@return TweenSequence
function Tween.Sequence() end

--- Stop a specific tween or sequence.
---@param tween TweenHandle|TweenSequence
function Tween.Kill(tween) end

--- Stop all active tweens and sequences.
function Tween.KillAll() end
