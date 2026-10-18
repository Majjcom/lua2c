local function native(x)
    local function identity(value) return value end
    local sum = 0
    for i = 1, x do sum = sum + i end
    return identity(sum)
end
for _, strip in ipairs({false, true}) do
    local ok, message = pcall(string.dump, native, strip)
    assert(not ok and message:match("unable to dump given function"))
end
local interpreted = assert(load("return function(x) return x + 3 end"))()
assert(assert(load(string.dump(interpreted)))(7) == 10)
assert(debug.getinfo(native, "S").what == "Lua")
local lines = debug.getinfo(native, "L").activelines
local stripped = debug.getinfo(native, "S").source == "=?"
assert((next(lines) == nil) == stripped)

-- Match all hook events against the interpreter, excluding inferred names and
-- normalizing line numbers to the beginning of each function definition.
local function work(x, ...)
    local sum = select("#", ...)
    for i = 1, x do sum = sum + i end
    return sum
end
local source = "return " .. [[function(x, ...)
    local sum = select("#", ...)
    for i = 1, x do sum = sum + i end
    return sum
end]]
local reference = assert(load(source))()
if stripped then reference = assert(load(string.dump(reference, true))) end
local function trace(fn)
    local events = {}
    local first = debug.getinfo(fn, "S").linedefined
    local function hook(event, line)
        if debug.getinfo(2, "f").func == fn then
            events[#events + 1] = event .. ":" .. (line and (line >= 0 and line - first or line) or "nil")
        end
    end
    debug.sethook(hook, "crl", 2)
    local result = fn(7, 1, 2)
    debug.sethook()
    assert(result == 30)
    return table.concat(events, ",")
end
local actual, expected = trace(work), trace(reference)
assert(actual == expected, actual .. "\nexpected: " .. expected)

-- Local/upvalue inspection and generic errors must not dereference absent code.
local captured = 5
local function inspect(argument)
    local info = debug.getinfo(1, "nSl")
    assert(info.what == "Lua" and (stripped or info.currentline > 0))
    local name, value = debug.getlocal(1, 1)
    assert(value == argument and (stripped or name == "argument"))
    return captured
end
assert(inspect(9) == 5)
local ok, message = pcall(function() local missing; return missing.field end)
assert(not ok and message:match("attempt to index a nil value"))
ok, message = pcall(function() local missing; return missing() end)
assert(not ok and message:match("attempt to call a nil value"))
collectgarbage("collect")
return native
