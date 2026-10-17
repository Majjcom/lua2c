local a = 0
local b = 0
local c = 1
local s = os.clock()
for i = 1, 10000000 do
    a = a + i
    b = b - a
    c = c * 1.00000001
end
print("a,b,c:", a, b, c)
--print("Time used:", os.clock() - s)
