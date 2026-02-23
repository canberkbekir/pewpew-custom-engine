---@meta

--- Field type helpers for declaring script inspector fields.
--- Use these in the `__fields` table of your script class to expose
--- editable properties in the CBEngine editor inspector.
---
--- Recommended style:
---   MyScript.__fields = {
---     speed = Field.Float(5.0, 0, 100),
---     offset = Field.Vector3(0, 1, 0),
---   }
---
--- Example:
--- ```lua
--- MyScript.__fields = {
---     speed = Field.Float(5.0, 0, 100),
---     name = Field.String("default"),
---     enabled = Field.Bool(true),
---     color = Field.Color(1, 0, 0, 1),
---     offset = Field.Vector3(0, 1, 0),
---     target = Field.EntityRef(),
---     targetRB = Field.ComponentRef(RigidBody),
---     targetScript = Field.ScriptRef(PlayerMovement),
--- }
--- ```

---@class Field
Field = {}

--- Declare a float field for the inspector.
---@param default? number Default value (default: 0)
---@param min? number Minimum slider value
---@param max? number Maximum slider value
---@return table
function Field.Float(default, min, max) end

--- Declare an integer field for the inspector.
---@param default? integer Default value (default: 0)
---@param min? integer Minimum slider value
---@param max? integer Maximum slider value
---@return table
function Field.Int(default, min, max) end

--- Declare a boolean field for the inspector.
---@param default? boolean Default value (default: false)
---@return table
function Field.Bool(default) end

--- Declare a string field for the inspector.
---@param default? string Default value (default: "")
---@return table
function Field.String(default) end

--- Declare a color field for the inspector (RGBA, 0.0-1.0).
---@param r? number Red (default: 1)
---@param g? number Green (default: 1)
---@param b? number Blue (default: 1)
---@param a? number Alpha (default: 1)
---@return table
function Field.Color(r, g, b, a) end

--- Declare a Vector3 field for the inspector.
---@param x? number X component (default: 0)
---@param y? number Y component (default: 0)
---@param z? number Z component (default: 0)
---@return table
function Field.Vector3(x, y, z) end

--- Declare an entity reference field.
--- Accepts drag-drop from the hierarchy panel.
--- At runtime the field receives the referenced Entity handle.
---@return table
function Field.EntityRef() end

--- Declare a component reference field.
--- Accepts drag-drop from the hierarchy panel, then resolves to the specified component proxy.
--- Pass the global component token (e.g., RigidBody, Transform, Camera).
--- At runtime the field receives the component proxy on the referenced entity.
---@param componentToken table A global component token (RigidBody, Transform, Camera, etc.)
---@return table
function Field.ComponentRef(componentToken) end

--- Declare a script reference field.
--- Accepts drag-drop from the hierarchy panel, then resolves to the specified script instance.
--- Pass the global script class table (e.g., PlayerMovement).
--- At runtime the field receives the script instance on the referenced entity.
---@param classTable table The global class table of the target script
---@return table
function Field.ScriptRef(classTable) end

--- Declare a blueprint reference field.
--- Accepts drag-drop of a .blueprint file from the content browser.
--- At runtime the field receives the blueprint file path as a string.
--- Pass the path to Scene:InstantiateBlueprint() to spawn it.
---@return table
function Field.BlueprintRef() end

--- Declare an audio clip reference field.
--- Accepts drag-drop of a .sfx asset from the content browser.
--- At runtime the field receives an AudioHandle.
---@return table
function Field.AudioRef() end

-- Back-compat global helpers (still supported)
function Float(default, min, max) end
function Int(default, min, max) end
function Bool(default) end
function String(default) end
function Color(r, g, b, a) end
function Vec3(x, y, z) end
function EntityRef() end
function ComponentRef(componentToken) end
function ScriptRef(classTable) end
function BlueprintRef() end
function AudioRef() end
