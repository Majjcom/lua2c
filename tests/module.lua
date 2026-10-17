local function make_counter()
    local n = 0
    return function(x) n = n + x; return n end
end
return { counter = make_counter(), square = function(x) return x*x end }
