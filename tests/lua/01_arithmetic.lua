-- Basic arithmetic and constant loading
local a = 1
local b = 2
local c = a + b
local d = a - b
local e = a * b
local f = a / b
local g = a % b
local h = a ^ b
local i = a // b

print(a, b, c, d, e, f, g, h, i)

-- Float operations
local x = 3.14
local y = 2.0
local z = x + y
print(z)

-- Unary operations
local neg = -a
local bnot = ~0xFF
local lnot = not true
local len = #"hello"
print(neg, bnot, lnot, len)
