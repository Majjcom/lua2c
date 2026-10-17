local mode, value = ...
value = tonumber(value)
if mode == "yield" then
    local result = coroutine.yield("paused", value)
    return result, value
end
if mode == "error" then error("entry expected") end
local function add(x) return x + value end
return add, mode
