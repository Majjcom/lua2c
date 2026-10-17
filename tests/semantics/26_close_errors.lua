local function resource(name)
    return setmetatable({name = name}, {
        __close = function(self, err) print("close", self.name, err ~= nil) end
    })
end
local function normal()
    local a <close> = resource("outer")
    do local b <close> = resource("inner") end
    return 1, 2, 3
end
print(normal())
local function failed()
    local a <close> = resource("failed")
    error("expected")
end
local ok = pcall(failed)
print("protected", ok)
local co = coroutine.create(function()
    local a <close> = setmetatable({}, { __close = function() coroutine.yield("closing") end })
    return "returned", 42
end)
print(coroutine.resume(co))
print(coroutine.resume(co))
local suspended = coroutine.create(function()
    local a <close> = resource("cancelled")
    coroutine.yield("suspended")
end)
print(coroutine.resume(suspended))
print(coroutine.close(suspended))
