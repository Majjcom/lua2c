-- Simple closure
local function make_adder(n)
    return function(x)
        return x + n
    end
end
local add5 = make_adder(5)
local add10 = make_adder(10)
print("add5(3):", add5(3))
print("add10(3):", add10(3))

-- Closure with mutable upvalue
local function make_counter(start)
    local count = start
    return function()
        count = count + 1
        return count
    end
end
local c1 = make_counter(0)
local c2 = make_counter(100)
print("c1:", c1(), c1(), c1())
print("c2:", c2(), c2())

-- Closure capturing multiple upvalues
local function make_point(x, y)
    local nx, ny = x, y
    return {
        getx = function() return nx end,
        gety = function() return ny end,
        move = function(dx, dy) nx = nx + dx; ny = ny + dy end,
    }
end
local p = make_point(3, 4)
print("point:", p.getx(), p.gety())
p.move(1, 2)
print("moved:", p.getx(), p.gety())

-- Nested closures
local function outer()
    local x = 1
    local function middle()
        local y = 2
        local function inner()
            return x + y
        end
        return inner
    end
    return middle
end
local m = outer()
local i = m()
print("nested closure:", i())

-- Closure in a loop
local fns = {}
for i = 1, 5 do
    fns[i] = function() return i end
end
print("loop closure:", fns[1](), fns[3](), fns[5]())
