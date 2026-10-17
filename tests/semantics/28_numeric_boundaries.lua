local min, max = math.mininteger, math.maxinteger
print("wrap", max + 1, min - 1, min * -1, -min)
print("division", -7 // 3, 7 // -3, -7 % 3, 7 % -3, min // -1, min % -1)
print("shift", 1 << 64, 1 >> 64, 8 << -1, -1 >> 63, 1 << min)
print("compare", max < max + 0.0, max == max + 0.0, min == min + 0.0)
local nan = 0/0
print("nan", nan == nan, nan < 1, nan <= 1)
print("coerce", "2" + 3, "6" // "2")
for i = max - 2, max do print("nearmax", i) end
for i = min + 2, min, -1 do print("nearmin", i) end
local function deep(n) if n == 0 then return "done" end return deep(n - 1) end
print("tail", deep(100000))
local ok = pcall(function() return 1 // 0 end)
print("zero", ok)
