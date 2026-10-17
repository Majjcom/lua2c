-- Deeply nested function definitions
local function level1()
    local v1 = 1
    local function level2()
        local v2 = 2
        local function level3()
            local v3 = 3
            return function()
                return v1 + v2 + v3
            end
        end
        return level3
    end
    return level2
end
local f = level1()()()
print("deep nesting:", f())

-- Table of functions
local ops = {
    add = function(a, b) return a + b end,
    sub = function(a, b) return a - b end,
    mul = function(a, b) return a * b end,
}
print("ops.add(3,4):", ops.add(3, 4))
print("ops.sub(10,3):", ops.sub(10, 3))

-- Array of functions
local transforms = {
    function(x) return x * 2 end,
    function(x) return x + 10 end,
    function(x) return -x end,
}
for i, fn in ipairs(transforms) do
    print("transform", i, fn(5))
end

-- Recursive data structure with closures
local function make_tree(value, left, right)
    return {
        value = value,
        left = left,
        right = right,
        sum = function(self)
            local s = self.value
            if self.left then s = s + self.left:sum() end
            if self.right then s = s + self.right:sum() end
            return s
        end,
    }
end

local tree = make_tree(10,
    make_tree(5, make_tree(2), make_tree(3)),
    make_tree(15, make_tree(12), make_tree(18))
)
print("tree sum:", tree:sum())
