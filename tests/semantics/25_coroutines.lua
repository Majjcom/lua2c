local function worker(n)
    local state = 0
    for i = 1, n do
        state = state + coroutine.yield(i, state)
    end
    return state, "done"
end
local co = coroutine.create(worker)
print(coroutine.resume(co, 3))
print(coroutine.resume(co, 10))
print(coroutine.resume(co, 20))
print(coroutine.resume(co, 30))
print(coroutine.status(co))
local t = setmetatable({}, {
    __index = function(_, key) return coroutine.yield("index", key) end,
    __add = function(a, b) return coroutine.yield("add") end,
    __lt = function(a, b) return coroutine.yield("lt") end,
    __concat = function(a, b) return coroutine.yield("concat") end,
})
local c = coroutine.create(function()
    print("index result", t.key)
    print("add result", t + t)
    print("lt result", t < t)
    print("concat result", "prefix" .. t .. "suffix")
end)
print(coroutine.resume(c))
print(coroutine.resume(c, 12))
print(coroutine.resume(c, 13))
print(coroutine.resume(c, true))
print(coroutine.resume(c, "value"))
local g = coroutine.create(function()
    for value in function() return coroutine.yield("iterator") end do
        print("iteration", value)
    end
end)
print(coroutine.resume(g))
print(coroutine.resume(g, 7))
print(coroutine.resume(g, nil))
