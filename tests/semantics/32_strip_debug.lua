local captured = 7
local function outer(value)
    local function inner(arg)
        local result = value + arg + captured
        return result
    end
    return inner
end

local inner = outer(10)
local info = debug.getinfo(inner, "SLu")
local stripped = next(info.activelines) == nil
assert(info.what == "Lua" and info.nparams == 1 and info.nups == 2)
assert(info.linedefined > 0 and info.lastlinedefined >= info.linedefined)
local local_name = debug.getlocal(inner, 1)
assert(stripped and local_name == nil or not stripped and local_name == "arg")
local upvalue_name = debug.getupvalue(inner, 1)
assert(stripped and upvalue_name == "(no name)" or not stripped and upvalue_name == "value")
assert(inner(5) == 22)

-- A dump/load round trip must retain the strip state of nested Lua functions.
local dumped = string.dump(outer)
local loaded = assert(load(dumped))
debug.setupvalue(loaded, 1, captured)
local reloaded = loaded(10)
assert(reloaded(5) == 22)
local reloaded_info = debug.getinfo(reloaded, "L")
assert((next(reloaded_info.activelines) == nil) == stripped)
assert((debug.getlocal(reloaded, 1) == nil) == stripped)

-- Stripping only affects the converted prototypes, not runtime-compiled code.
local dynamic = assert(load("return function(dynamic_arg) return dynamic_arg end"))()
assert(debug.getlocal(dynamic, 1) == "dynamic_arg")
print("strip", stripped, inner(5), reloaded(5), info.linedefined, info.lastlinedefined)
