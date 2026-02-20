---@meta

--- Input polling API for keyboard and mouse state.
---@class Input
Input = {}

--- Check if a keyboard key is currently pressed.
---@param keyCode integer Use Key.* constants (e.g., Key.W, Key.Space)
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

-- =====================
-- Key Constants
-- =====================

--- Keyboard key codes for use with Input.IsKeyPressed().
---@class Key
---@field Space integer
---@field Escape integer
---@field Enter integer
---@field Tab integer
---@field W integer
---@field A integer
---@field S integer
---@field D integer
---@field Q integer
---@field E integer
---@field R integer
---@field F integer
---@field Up integer
---@field Down integer
---@field Left integer
---@field Right integer
---@field LeftShift integer
---@field LeftControl integer
---@field LeftAlt integer
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
