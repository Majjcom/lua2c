-- String concatenation
local s1 = "hello"
local s2 = " world"
local s3 = s1 .. s2
print("concat:", s3)

-- String length
print("length:", #s3)

-- Multi-part concatenation
local a = "a"
local b = "b"
local c = "c"
local abc = a .. b .. c
print("abc:", abc)

-- String comparison
local s = "abc"
if s == "abc" then
    print("string eq works")
end

if s ~= "def" then
    print("string ne works")
end

-- String from number
local num_str = tostring(42)
print("tostring:", num_str)

-- Number from string
local str_num = tonumber("123")
print("tonumber:", str_num + 1)

-- String with escapes
local escaped = "line1\nline2\ttab"
print("escaped:", escaped)
