---@meta

--- Capitalized math utility API. Mirrors the built-in `math` library but with
--- consistent PascalCase naming plus engine-specific helpers.
---@class Math
Math = {}

--- Pi constant (~3.14159).
---@type number
Math.Pi = 3.14159265358979323846

-- =====================
-- Rounding / Integer
-- =====================

---@param x number
---@return number
function Math.Floor(x) end

---@param x number
---@return number
function Math.Ceil(x) end

---@param x number
---@return number
function Math.Round(x) end

---@param x number
---@return number
function Math.Abs(x) end

--- Returns 1, -1, or 0 depending on the sign of x.
---@param x number
---@return number
function Math.Sign(x) end

-- =====================
-- Range / Interpolation
-- =====================

---@param x number
---@param min number
---@param max number
---@return number
function Math.Clamp(x, min, max) end

---@param a number
---@param b number
---@param t number Interpolation factor (0=a, 1=b)
---@return number
function Math.Lerp(a, b, t) end

---@param a number
---@param b number
---@return number
function Math.Min(a, b) end

---@param a number
---@param b number
---@return number
function Math.Max(a, b) end

--- Hermite interpolation. Returns 0 when x <= edge0 and 1 when x >= edge1.
---@param edge0 number
---@param edge1 number
---@param x number
---@return number
function Math.Smoothstep(edge0, edge1, x) end

--- Move `current` towards `target` by at most `maxDelta`, without overshooting.
---@param current number
---@param target number
---@param maxDelta number
---@return number
function Math.MoveTowards(current, target, maxDelta) end

--- Ping-pong t between 0 and length.
---@param t number
---@param length number
---@return number
function Math.PingPong(t, length) end

-- =====================
-- Power / Log
-- =====================

---@param x number
---@return number
function Math.Sqrt(x) end

---@param x number
---@param y number
---@return number
function Math.Pow(x, y) end

--- e^x
---@param x number
---@return number
function Math.Exp(x) end

--- Natural logarithm.
---@param x number
---@return number
function Math.Log(x) end

-- =====================
-- Trigonometry
-- =====================

---@param x number Angle in radians
---@return number
function Math.Sin(x) end

---@param x number Angle in radians
---@return number
function Math.Cos(x) end

---@param x number Angle in radians
---@return number
function Math.Tan(x) end

---@param x number
---@return number
function Math.Asin(x) end

---@param x number
---@return number
function Math.Acos(x) end

---@param y number
---@param x number
---@return number
function Math.Atan2(y, x) end

--- Convert degrees to radians.
---@param deg number
---@return number
function Math.Rad(deg) end

--- Convert radians to degrees.
---@param rad number
---@return number
function Math.Deg(rad) end
