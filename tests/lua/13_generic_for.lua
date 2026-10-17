-- Generic for with pairs
local t = {x = 1, y = 2, z = 3}
local keys = {}
local vals = {}
for k, v in pairs(t) do
    keys[#keys + 1] = k
    vals[#vals + 1] = v
end
print("pairs count:", #keys, #vals)

-- Generic for with ipairs
local arr = {10, 20, 30, 40}
local arr_sum = 0
for i, v in ipairs(arr) do
    arr_sum = arr_sum + v
end
print("ipairs sum:", arr_sum)

-- Custom iterator (stateless)
local function range_iter(t, i)
    i = i + 1
    if i <= t.n then
        return i, t[i]
    end
end
local function range(n)
    local t = {}
    t.n = n
    for i = 1, n do t[i] = i * 10 end
    return range_iter, t, 0
end
local range_sum = 0
for i, v in range(5) do
    range_sum = range_sum + v
end
print("range sum:", range_sum)

-- Generic for modifying table
local data = {a = 10, b = 20, c = 30}
local total = 0
for k, v in pairs(data) do
    total = total + v
end
print("data total:", total)
