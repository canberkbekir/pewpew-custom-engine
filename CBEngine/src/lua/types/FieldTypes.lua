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
---     target = EntityRef(),
---     targetRB = ComponentRef(RigidBody),
---     targetScript = ScriptRef(PlayerMovement),
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

--- Declare an entity reference field.
--- Accepts drag-drop from the hierarchy panel.
--- At runtime the field receives the referenced Entity handle.
---@return table
function EntityRef() end

--- Declare a component reference field.
--- Accepts drag-drop from the hierarchy panel, then resolves to the specified component proxy.
--- Pass the global component token (e.g., RigidBody, Transform, Camera).
--- At runtime the field receives the component proxy on the referenced entity.
---@param componentToken table A global component token (RigidBody, Transform, Camera, etc.)
---@return table
function ComponentRef(componentToken) end

--- Declare a script reference field.
--- Accepts drag-drop from the hierarchy panel, then resolves to the specified script instance.
--- Pass the global script class table (e.g., PlayerMovement).
--- At runtime the field receives the script instance on the referenced entity.
---@param classTable table The global class table of the target script
---@return table
function ScriptRef(classTable) end

--- Declare a blueprint reference field.
--- Accepts drag-drop of a .blueprint file from the content browser.
--- At runtime the field receives the blueprint file path as a string.
--- Pass the path to Scene:InstantiateBlueprint() to spawn it.
---@return table
function BlueprintRef() end
