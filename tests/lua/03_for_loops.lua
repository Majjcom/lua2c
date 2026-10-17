-- Numeric for loop (ascending)
local sum = 0
for i = 1, 10 do
    sum = sum + i
end
print("1+..+10 =", sum)

-- Numeric for loop with step
local even_sum = 0
for i = 2, 10, 2 do
    even_sum = even_sum + i
end
print("even sum:", even_sum)

-- Numeric for loop (descending)
local dec = ""
for i = 5, 1, -1 do
    dec = dec .. i
end
print("descending:", dec)

-- Float for loop
local fsum = 0.0
for x = 0.0, 1.0, 0.25 do
    fsum = fsum + x
end
print("float sum:", fsum)

-- Nested for loops
local matrix_sum = 0
for i = 1, 3 do
    for j = 1, 3 do
        matrix_sum = matrix_sum + i * j
    end
end
print("matrix sum:", matrix_sum)
