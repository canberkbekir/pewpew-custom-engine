---@meta

--- Input polling API for keyboard and mouse state.
---@class Input
Input = {}

--- Check if a keyboard or mouse key is currently held.
---@param keyCode integer Use Key.* constants (e.g., Key.W, Key.Space, Key.MouseLeft)
---@return boolean
function Input.IsKeyPressed(keyCode) end

--- Check if a mouse button is currently pressed.
---@param button integer Use Mouse.* constants (e.g., Mouse.Left)
---@return boolean
function Input.IsMouseButtonPressed(button) end

--- Get the current mouse position in screen coordinates.
---@return number x, number y
function Input.GetMousePosition() end

--- Get the current mouse X position in screen coordinates.
---@return number
function Input.GetMouseX() end

--- Get the current mouse Y position in screen coordinates.
---@return number
function Input.GetMouseY() end

--- Check if a key was pressed this frame (true for one frame only).
--- Use this for single-shot actions like jump or shoot.
---@param keyCode integer Use Key.* constants
---@return boolean
function Input.IsKeyJustPressed(keyCode) end

--- Check if a key was released this frame (true for one frame only).
---@param keyCode integer Use Key.* constants
---@return boolean
function Input.IsKeyJustReleased(keyCode) end

--- Get mouse movement since last frame as a Vector2.
--- When cursor is locked, returns raw unaccelerated hardware delta.
---@return Vector2
function Input.GetMouseDelta() end

--- Get mouse scroll wheel delta this frame as a Vector2.
--- x = horizontal scroll, y = vertical scroll.
---@return Vector2
function Input.GetMouseScrollDelta() end

--- Lock/hide the cursor for FPS-style look.
--- Also enables raw mouse motion (bypasses OS pointer acceleration).
---@param locked boolean
function Input.SetCursorLocked(locked) end

--- Returns true if the cursor is currently locked.
---@return boolean
function Input.IsCursorLocked() end

-- =====================
-- Key Constants
-- =====================

--- Keyboard and mouse button codes for use with Input.IsKeyPressed().
---@class Key
---@field Space integer
---@field Escape integer
---@field Enter integer
---@field Tab integer
---@field Backspace integer
---@field Insert integer
---@field Delete integer
---@field PageUp integer
---@field PageDown integer
---@field Home integer
---@field End integer
---@field CapsLock integer
---@field ScrollLock integer
---@field NumLock integer
---@field PrintScreen integer
---@field Pause integer
---@field Up integer
---@field Down integer
---@field Left integer
---@field Right integer
---@field A integer @...@field Z integer
---@field Num0 integer @...@field Num9 integer
---@field F1 integer @...@field F12 integer
---@field LeftShift integer
---@field LeftControl integer
---@field LeftAlt integer
---@field LeftSuper integer
---@field RightShift integer
---@field RightControl integer
---@field RightAlt integer
---@field RightSuper integer
---@field KP0 integer @...@field KP9 integer
---@field KPDecimal integer
---@field KPDivide integer
---@field KPMultiply integer
---@field KPSubtract integer
---@field KPAdd integer
---@field KPEnter integer
---@field KPEqual integer
---@field MouseLeft integer   Mouse left button
---@field MouseRight integer  Mouse right button
---@field MouseMiddle integer Mouse middle button
---@field MouseX1 integer     Mouse side button 1
---@field MouseX2 integer     Mouse side button 2
Key = {}

-- =====================
-- Mouse Constants
-- =====================

--- Mouse button codes for use with Input.IsMouseButtonPressed().
---@class Mouse
---@field Left integer
---@field Right integer
---@field Middle integer
Mouse = {}
