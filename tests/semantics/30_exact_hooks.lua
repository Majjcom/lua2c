local events = {}
local function hooked(event, line)
    local info = debug.getinfo(2, "nS")
    events[#events + 1] = event .. ":" .. tostring(line) .. ":" .. (info.what or "?") .. ":" .. (info.name or "?")
end
local function square(x) return x*x end
local function iterator(_, x) if x < 3 then return x + 1 end end
local function work(...)
    local total = select("#", ...)
    for x in iterator, nil, 0 do total = total + square(x) end
    for i = 1, 2 do total = total + i end
    local obj = setmetatable({}, { __add = function() return 5 end })
    total = total + (obj + obj)
    return total
end
debug.sethook(hooked, "crl", 2)
local result = work("alpha", "beta")
debug.sethook()
print("result", result)
for _, event in ipairs(events) do print(event) end
