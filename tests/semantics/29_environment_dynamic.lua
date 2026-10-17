local saved = _ENV
local function scoped()
    local _ENV = { x = 4, print = saved.print }
    local function f() x = x + 1; return x end
    print("env", f(), f())
end
scoped()
local dynamic = assert(load("return function(n) return n * 3 end"))()
print("dynamic", dynamic(7))
local function caller(f, ...) return f(...) end
print("mixed", caller(dynamic, 8))
local binary = "a\0b\255"
print("binary", #binary, string.byte(binary, 1, #binary))
print("varargs", ...)
