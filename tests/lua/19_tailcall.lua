-- Tail call: recursive without stack growth
local function tail_fact(n, acc)
    acc = acc or 1
    if n <= 1 then
        return acc
    end
    return tail_fact(n - 1, acc * n)
end
print("tail_fact(10):", tail_fact(10))

-- Tail call between two functions (forward declaration via global)
is_odd = nil  -- forward declaration
local function is_even(n)
    if n == 0 then return true end
    return is_odd(n - 1)
end
function is_odd(n)
    if n == 0 then return false end
    return is_even(n - 1)
end
print("is_even(4):", is_even(4))
print("is_odd(7):", is_odd(7))

-- Simple tail call
local function chain(n)
    if n <= 0 then return "done" end
    return chain(n - 1)
end
print("chain:", chain(5))
