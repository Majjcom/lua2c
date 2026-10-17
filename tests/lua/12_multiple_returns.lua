-- Multiple return values
local function divmod(a, b)
    return a // b, a % b
end
local q, r = divmod(17, 5)
print("divmod:", q, r)

-- Only take first return
local first = divmod(10, 3)
print("first only:", first)

-- Multiple returns in expressions
local function minmax(a, b)
    if a < b then return a, b else return b, a end
end
local lo, hi = minmax(7, 3)
print("minmax:", lo, hi)

-- Return multiple values from table unpack
local function unpack_test()
    local t = {10, 20, 30}
    return table.unpack(t)
end
local a, b, c = unpack_test()
print("unpack:", a, b, c)

-- Multiple returns used as arguments
local function add3(a, b, c)
    return a + b + c
end
local function three_vals()
    return 1, 2, 3
end
print("add3(three_vals):", add3(three_vals()))

-- Return in the middle of function
local function abs(x)
    if x < 0 then
        return -x
    end
    return x
end
print("abs(-5):", abs(-5))
print("abs(5):", abs(5))
