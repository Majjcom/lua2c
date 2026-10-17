-- Boolean and nil values
local t = true
local f = false
local n = nil

print("true:", t)
print("false:", f)
print("nil:", n)

-- Truthiness: only nil and false are falsy
if 0 then print("0 is truthy") end
if "" then print("empty string is truthy") end
if {} then print("empty table is truthy") end
if not nil then print("not nil is truthy") end
if not false then print("not false is truthy") end

-- Nil default for uninitialized
local uninitialized
print("uninitialized:", uninitialized)

-- Nil comparison
if n == nil then
    print("is nil")
end

-- Boolean operations with nil
print("nil and 1:", n and 1)
print("nil or 1:", n or 1)
print("false or nil:", f or n)

-- Loading nil into register
local a, b, c = nil, 42, nil
print("a,b,c:", a, b, c)
