-- Logical operators (and, or, not)
local t = true
local f = false
local n = nil

print("not true:", not t)
print("not false:", not f)
print("not nil:", not n)
print("not 0:", not 0)

-- Short-circuit and
print("true and 1:", t and 1)
print("false and 1:", f and 1)
print("nil and 1:", n and 1)

-- Short-circuit or
print("true or 1:", t or 1)
print("false or 1:", f or 1)
print("nil or 1:", n or 1)

-- Default value pattern
local function get_name(name)
    return name or "anonymous"
end
print("get_name:", get_name(nil))
print("get_name:", get_name("Bob"))

-- Ternary-like pattern
local function max(a, b)
    return (a > b) and a or b
end
print("max(3,5):", max(3, 5))
print("max(5,3):", max(5, 3))

-- Chained or
local function first_non_nil(...)
    return ... or first_non_nil(select(2, ...))
end
-- Simple chained or
local val = nil or false or 42
print("chained or:", val)
