-- Basic function definition and call
local function add(a, b)
    return a + b
end
print("add:", add(3, 4))

-- Function with multiple returns
local function swap(a, b)
    return b, a
end
local x, y = swap(1, 2)
print("swap:", x, y)

-- Function with no return
local function greet(name)
    print("hello " .. name)
end
greet("world")

-- Recursive function (factorial)
local function fact(n)
    if n <= 1 then
        return 1
    end
    return n * fact(n - 1)
end
print("fact(5):", fact(5))

-- Fibonacci
local function fib(n)
    if n <= 1 then
        return n
    end
    return fib(n - 1) + fib(n - 2)
end
print("fib(10):", fib(10))

-- Function as argument
local function apply(f, x)
    return f(x)
end
local function double(n)
    return n * 2
end
print("apply double:", apply(double, 5))

-- Function returning function
local function multiplier(factor)
    return function(x)
        return x * factor
    end
end
local triple = multiplier(3)
print("triple:", triple(7))

-- Vararg function
local function va_sum(...)
    local s = 0
    local args = {...}
    for i = 1, #args do
        s = s + args[i]
    end
    return s
end
print("va_sum:", va_sum(1, 2, 3, 4, 5))

-- Select with varargs
local function first_and_last(...)
    return select(1, ...), select(-1, ...)
end
print("first_and_last:", first_and_last(10, 20, 30))
