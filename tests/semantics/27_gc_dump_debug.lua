local function make()
    local x = 0
    return function(n) x = x + n; return x end,
           function() return x end
end
local a, b = make()
for i = 1, 100 do
    local t = {}
    for j = 1, 100 do t[j] = string.rep("x", j) end
    collectgarbage("collect")
    a(i)
end
print("shared", b())
local function pure(n) return n * n, n + 1 end
local loaded = assert(load(string.dump(pure)))
print("dump", loaded(9))
local info = debug.getinfo(pure, "Su")
print("identity", info.what, info.nparams, info.isvararg)
local _, up = debug.getupvalue(a, 1)
debug.setupvalue(a, 1, up + 1)
print("upvalue", b(), debug.upvalueid(a, 1) == debug.upvalueid(b, 1))
local count, lines = 0, 0
debug.sethook(function(event)
    if event == "count" then count = count + 1 end
    if event == "line" then lines = lines + 1 end
end, "l", 1)
local total = 0
for i = 1, 10 do total = total + pure(i) end
debug.sethook()
print("hooks", count > 0, lines > 0, total)
