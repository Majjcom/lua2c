-- Table constructor variations
local t1 = {}                           -- empty
local t2 = {1, 2, 3}                   -- array only
local t3 = {a=1, b=2, c=3}             -- hash only
local t4 = {1, 2, x=10, 3, y=20}       -- mixed
local t5 = {                            -- multi-line
    "alpha",
    "beta",
    key1 = "val1",
    key2 = "val2",
}

print("#t2:", #t2)
print("t3.a:", t3.a)
print("t4[1]:", t4[1], "t4.x:", t4.x)
print("t5[1]:", t5[1], "t5.key1:", t5.key1)

-- Table with trailing comma
local t6 = {
    10,
    20,
    30,
}
print("#t6:", #t6)

-- Nested table constructor
local config = {
    server = {
        host = "localhost",
        port = 8080,
    },
    database = {
        name = "test",
        options = {
            timeout = 30,
            retry = true,
        },
    },
}
print("host:", config.server.host)
print("port:", config.server.port)
print("db name:", config.database.name)
print("timeout:", config.database.options.timeout)

-- Table with computed keys
local key = "dynamic"
local t7 = {
    [key] = 100,
    [1+1] = 200,
}
print("dynamic key:", t7.dynamic)
print("computed key:", t7[2])

-- Table with many elements (tests SETLIST)
local big = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}
local big_sum = 0
for i = 1, #big do big_sum = big_sum + big[i] end
print("big sum:", big_sum)
