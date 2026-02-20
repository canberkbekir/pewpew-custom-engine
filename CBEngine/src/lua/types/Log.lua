---@meta

--- Logging functions. Messages appear in the engine console with a [Lua] prefix.
---@class Log
Log = {}

--- Log an informational message.
---@param message string
function Log.Info(message) end

--- Log a warning message.
---@param message string
function Log.Warn(message) end

--- Log an error message.
---@param message string
function Log.Error(message) end

--- Log a trace-level message (verbose).
---@param message string
function Log.Trace(message) end
