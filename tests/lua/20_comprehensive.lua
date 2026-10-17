-- Comprehensive test combining many features

-- Utility functions
local function map(t, f)
    local result = {}
    for i, v in ipairs(t) do
        result[i] = f(v)
    end
    return result
end

local function filter(t, pred)
    local result = {}
    for i, v in ipairs(t) do
        if pred(v) then
            result[#result + 1] = v
        end
    end
    return result
end

local function reduce(t, init, f)
    local acc = init
    for i, v in ipairs(t) do
        acc = f(acc, v)
    end
    return acc
end

-- Test data
local nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}

-- Filter even, map to square, reduce with sum
local evens = filter(nums, function(x) return x % 2 == 0 end)
local squares = map(evens, function(x) return x * x end)
local total = reduce(squares, 0, function(a, b) return a + b end)
print("sum of squares of evens:", total)

-- String processing
local function split_words(s)
    local words = {}
    local i = 1
    while true do
        local start, finish = s:find("(%S+)", i)
        if not start then break end
        words[#words + 1] = s:sub(start, finish)
        i = finish + 1
    end
    return words
end

local sentence = "the quick brown fox jumps over the lazy dog"
local words = split_words(sentence)
print("word count:", #words)

-- Simple class-like pattern
local function Animal(name, sound)
    return {
        name = name,
        sound = sound,
        speak = function(self)
            print(self.name .. " says " .. self.sound)
        end,
        greet = function(self, other)
            print(self.name .. " greets " .. other.name)
        end,
    }
end

local cat = Animal("Cat", "meow")
local dog = Animal("Dog", "woof")
cat:speak()
dog:speak()
cat:greet(dog)

-- Memoization with closures
local function memoize(f)
    local cache = {}
    return function(x)
        if cache[x] ~= nil then
            return cache[x]
        end
        local result = f(x)
        cache[x] = result
        return result
    end
end

-- Use global for self-reference in memoized fib
local memo_fib
memo_fib = memoize(function(n)
    if n <= 1 then return n end
    return memo_fib(n - 1) + memo_fib(n - 2)
end)
print("memo_fib(20):", memo_fib(20))

-- Complex scope interactions
local value = 0
local function increment(n)
    value = value + n
    return value
end
print("increment:", increment(5))
print("increment:", increment(3))
print("final value:", value)
