-- Global variable assignment and access
global_var = 100
print("global:", global_var)

-- Multiple global assignments
g1, g2, g3 = 1, 2, 3
print("globals:", g1, g2, g3)

-- Local variable shadowing
local x = 1
do
    local x = 2
    print("inner x:", x)
end
print("outer x:", x)

-- Local in different scopes
do
    local a = 10
    print("scope a:", a)
end
do
    local a = 20
    print("scope a:", a)
end

-- Mixed global and local
local loc = "local"
glob = "global"
print(loc, glob)

-- Function accessing global
function modify_global()
    global_var = global_var + 1
end
modify_global()
print("after modify:", global_var)

-- Local function vs global function
local function local_func()
    return "from local"
end
function global_func()
    return "from global"
end
print(local_func())
print(global_func())
