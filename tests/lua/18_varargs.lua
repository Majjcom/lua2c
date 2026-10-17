-- Simple varargs
local function va_print(...)
    local args = {...}
    for i = 1, #args do
        io.write(tostring(args[i]))
        if i < #args then io.write(" ") end
    end
    io.write("\n")
end
va_print(1, "two", 3.0)

-- Varargs forwarding
local function va_sum(...)
    local args = {...}
    local s = 0
    for i = 1, #args do
        s = s + args[i]
    end
    return s
end
print("va_sum:", va_sum(1, 2, 3, 4))

-- Varargs with fixed params
local function named_va(name, ...)
    print("name:", name)
    print("extra:", va_sum(...))
end
named_va("test", 10, 20, 30)

-- Vararg count
local function va_count(...)
    return select("#", ...)
end
print("va_count:", va_count(1, 2, 3, 4, 5))

-- Varargs as table
local function pack(...)
    return {...}
end
local packed = pack("a", "b", "c")
print("packed:", #packed, packed[1], packed[3])

-- Mixed return and varargs
local function first(...)
    return ...
end
print("first:", first(10, 20, 30))
