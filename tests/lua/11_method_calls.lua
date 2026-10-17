-- Method definition with colon syntax
local obj = {
    x = 0,
    y = 0,
}

function obj:set(nx, ny)
    self.x = nx
    self.y = ny
end

function obj:getx()
    return self.x
end

function obj:gety()
    return self.y
end

function obj:move(dx, dy)
    self.x = self.x + dx
    self.y = self.y + dy
end

obj:set(3, 4)
print("x:", obj:getx())
print("y:", obj:gety())
obj:move(1, 2)
print("after move x:", obj:getx())
print("after move y:", obj:gety())

-- Method with string key via dot
function obj.dump(self)
    return "(" .. self.x .. "," .. self.y .. ")"
end
print("dump:", obj:dump())

-- Chained method calls
local builder = {
    result = "",
}
function builder:add(s)
    self.result = self.result .. s
    return self
end
function builder:build()
    return self.result
end

local s = builder:add("a"):add("b"):add("c"):build()
print("builder:", s)

-- Table with methods as constructor
local function make_stack()
    local stack = {
        data = {},
    }
    function stack:push(val)
        self.data[#self.data + 1] = val
    end
    function stack:pop()
        local val = self.data[#self.data]
        self.data[#self.data] = nil
        return val
    end
    function stack:isempty()
        return #self.data == 0
    end
    return stack
end

local st = make_stack()
st:push(10)
st:push(20)
st:push(30)
print("pop:", st:pop())
print("pop:", st:pop())
print("empty:", st:isempty())
