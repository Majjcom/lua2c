local a = {
    val = 2
}
local b = {
    val = 3.3
}

local function meta_add(x, y)
    return x.val + y.val
end

setmetatable(a, {
    __add = meta_add
})
setmetatable(b, {
    __add = meta_add
})

print("a + b:", a + b)
print("b + a:", b + a)
