-- Basic table creation and access
local t = {}
t.x = 10
t.y = 20
print("t.x:", t.x, "t.y:", t.y)

-- Table with constructor
local colors = {"red", "green", "blue"}
print("colors[1]:", colors[1])
print("colors[3]:", colors[3])

-- Mixed table constructor
local person = {
    name = "Alice",
    age = 30,
    "item1",
    "item2",
}
print("name:", person.name)
print("age:", person.age)
print("[1]:", person[1])

-- Table modification
person.email = "alice@example.com"
person.age = 31
print("email:", person.email)
print("new age:", person.age)

-- Nested tables
local matrix = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
}
print("matrix[2][3]:", matrix[2][3])

-- Table length
local arr = {10, 20, 30, 40, 50}
print("#arr:", #arr)

-- Generic for with pairs
local dict = {a = 1, b = 2, c = 3}
local total = 0
for k, v in pairs(dict) do
    total = total + v
end
print("dict total:", total)

-- Generic for with ipairs
local items = {"apple", "banana", "cherry"}
for i, v in ipairs(items) do
    print("item", i, v)
end

-- Table insert via SETLIST
local big = {}
for i = 1, 10 do
    big[i] = i * i
end
print("big[5]:", big[5])

-- Self-referencing table field
local node = {}
node.value = 42
node.next = node
print("node.next.value:", node.next.value)
