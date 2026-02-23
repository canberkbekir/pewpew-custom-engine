---@meta

-- =====================================================================
-- Easing — pre-built easing functions for use with TweenHandle:SetEase()
-- All functions take a normalized time t in [0, 1] and return a value in [0, 1].
-- =====================================================================

---@class Easing
Easing = {}

-- ---- Linear ----

--- No easing — constant rate of change.
---@type fun(t: number): number
Easing.Linear = nil

-- ---- Quadratic ----

--- Quadratic ease-in (accelerate from zero).
---@type fun(t: number): number
Easing.QuadIn = nil

--- Quadratic ease-out (decelerate to zero).
---@type fun(t: number): number
Easing.QuadOut = nil

--- Quadratic ease-in then ease-out.
---@type fun(t: number): number
Easing.QuadInOut = nil

-- ---- Cubic ----

--- Cubic ease-in (accelerate from zero, stronger than quad).
---@type fun(t: number): number
Easing.CubicIn = nil

--- Cubic ease-out (decelerate to zero, stronger than quad).
---@type fun(t: number): number
Easing.CubicOut = nil

--- Cubic ease-in then ease-out.
---@type fun(t: number): number
Easing.CubicInOut = nil

-- ---- Sine ----

--- Sinusoidal ease-in (gentle acceleration).
---@type fun(t: number): number
Easing.SineIn = nil

--- Sinusoidal ease-out (gentle deceleration).
---@type fun(t: number): number
Easing.SineOut = nil

--- Sinusoidal ease-in then ease-out.
---@type fun(t: number): number
Easing.SineInOut = nil

-- ---- Exponential ----

--- Exponential ease-in (dramatic acceleration from near-zero).
---@type fun(t: number): number
Easing.ExpoIn = nil

--- Exponential ease-out (dramatic deceleration to near-zero).
---@type fun(t: number): number
Easing.ExpoOut = nil

--- Exponential ease-in then ease-out.
---@type fun(t: number): number
Easing.ExpoInOut = nil

-- ---- Circular ----

--- Circular ease-in.
---@type fun(t: number): number
Easing.CircIn = nil

--- Circular ease-out.
---@type fun(t: number): number
Easing.CircOut = nil

--- Circular ease-in then ease-out.
---@type fun(t: number): number
Easing.CircInOut = nil

-- ---- Back (overshoot) ----

--- Backs up slightly before moving forward.
---@type fun(t: number): number
Easing.BackIn = nil

--- Overshoots the target before settling.
---@type fun(t: number): number
Easing.BackOut = nil

--- Back ease-in then ease-out (back + overshoot).
---@type fun(t: number): number
Easing.BackInOut = nil

-- ---- Bounce ----

--- Bounce ease-out (bounces at the end). Alias: Easing.Bounce.
---@type fun(t: number): number
Easing.BounceOut = nil

--- Bounce ease-in (bounces at the start).
---@type fun(t: number): number
Easing.BounceIn = nil

--- Bounce ease-in then ease-out.
---@type fun(t: number): number
Easing.BounceInOut = nil

--- Alias for BounceOut (backward compatibility).
---@type fun(t: number): number
Easing.Bounce = nil

-- ---- Elastic ----

--- Elastic ease-out (springy overshoot at end). Alias: Easing.Elastic.
---@type fun(t: number): number
Easing.ElasticOut = nil

--- Elastic ease-in (springy wind-up at start).
---@type fun(t: number): number
Easing.ElasticIn = nil

--- Elastic ease-in then ease-out.
---@type fun(t: number): number
Easing.ElasticInOut = nil

--- Alias for ElasticOut (backward compatibility).
---@type fun(t: number): number
Easing.Elastic = nil

-- ---- Classic aliases (backward compatibility) ----

--- Alias for QuadIn.
---@type fun(t: number): number
Easing.EaseIn = nil

--- Alias for QuadOut.
---@type fun(t: number): number
Easing.EaseOut = nil

--- Alias for QuadInOut.
---@type fun(t: number): number
Easing.EaseInOut = nil
