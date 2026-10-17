local function yielding(value) coroutine.yield(value); return value end
local obj = setmetatable({}, {
    __newindex = function(t, k, v) yielding("set:" .. k); rawset(t, k, v) end,
    __len = function() return yielding(42) end,
    __unm = function() return yielding(-8) end,
    __bnot = function() return yielding(123) end,
    __eq = function() return yielding(true) end,
    __le = function() return yielding(true) end,
})
local other = setmetatable({}, getmetatable(obj))
local co = coroutine.create(function()
    obj.key = 7
    print("assignment", obj.key)
    print("length", #obj)
    print("unary", -obj)
    print("bitnot", ~obj)
    print("equal", obj == other)
    print("order", obj <= other)
    local ok, value = pcall(function() return yielding("protected") end)
    print("pcall", ok, value)
    local resource <close> = setmetatable({}, {__close = function() yielding("closed") end})
    return "done"
end)
while coroutine.status(co) ~= "dead" do print(coroutine.resume(co)) end
local function tail(n)
    if n == 0 then return yielding("tail") end
    return tail(n - 1)
end
local t = coroutine.create(function() return tail(10000) end)
print(coroutine.resume(t))
print(coroutine.resume(t))
