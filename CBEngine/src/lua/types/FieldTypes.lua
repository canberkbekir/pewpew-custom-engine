---@meta

--- Field type helpers for declaring script inspector fields.
--- Use these in the `__fields` table of your script class to expose
--- editable properties in the CBEngine editor inspector.
---
--- Example:
--- ```lua
--- MyScript.__fields = {
---     speed = Float(5.0, 0, 100),
---     name = String("default"),
---     enabled = Bool(true),
---     color = Color(1, 0, 0, 1),
---     offset = Vector3(0, 1, 0),
--- }
--- ```

--- Declare a float field for the inspector.
---@param default? number Default value (default: 0)
---@param min? number Minimum slider value
---@param max? number Maximum slider value
---@return table
function Float(default, min, max) end

--- Declare an integer field for the inspector.
---@param default? integer Default value (default: 0)
---@param min? integer Minimum slider value
---@param max? integer Maximum slider value
---@return table
function Int(default, min, max) end

--- Declare a boolean field for the inspector.
---@param default? boolean Default value (default: false)
---@return table
function Bool(default) end

--- Declare a string field for the inspector.
---@param default? string Default value (default: "")
---@return table
function String(default) end

--- Declare a color field for the inspector (RGBA, 0.0-1.0).
---@param r? number Red (default: 1)
---@param g? number Green (default: 1)
---@param b? number Blue (default: 1)
---@param a? number Alpha (default: 1)
---@return table
function Color(r, g, b, a) end

--- Declare a Vector3 field for the inspector.
---@param x? number X component (default: 0)
---@param y? number Y component (default: 0)
---@param z? number Z component (default: 0)
---@return table
function Vector3(x, y, z) end
