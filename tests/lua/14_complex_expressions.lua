-- Complex expressions and operator precedence
local a = 2 + 3 * 4
print("2+3*4:", a)

local b = (2 + 3) * 4
print("(2+3)*4:", b)

local c = 2 ^ 3 ^ 2
print("2^3^2:", c)

-- Mixed integer and float
local m1 = 3 / 2
local m2 = 3 // 2
print("3/2:", m1, "3//2:", m2)

-- Compound expressions
local x = 1
x = x + 1
x = x * 2
x = x - 3
print("compound:", x)

-- Comparison chains (not directly supported, uses and)
local function in_range(val, lo, hi)
    return val >= lo and val <= hi
end
print("5 in [1,10]:", in_range(5, 1, 10))
print("15 in [1,10]:", in_range(15, 1, 10))

-- String/number coercion in arithmetic
local s = "10"
local n = s + 5
print("str+num:", n)

-- Ternary pattern with and/or
local function clamp(val, lo, hi)
    return (val < lo and lo) or (val > hi and hi) or val
end
print("clamp(-5,0,10):", clamp(-5, 0, 10))
print("clamp(5,0,10):", clamp(5, 0, 10))
print("clamp(15,0,10):", clamp(15, 0, 10))

-- Nested function calls in expressions
local function f(x) return x * x end
local function g(x) return x + 1 end
print("f(g(3)):", f(g(3)))
