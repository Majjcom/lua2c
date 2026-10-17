-- If/else/elseif
local x = 10

if x > 15 then
    print("big")
elseif x > 5 then
    print("medium")
else
    print("small")
end

-- Nested if
local a = 1
local b = 2
if a == 1 then
    if b == 2 then
        print("both match")
    else
        print("only a matches")
    end
end

-- While loop
local count = 0
while count < 5 do
    count = count + 1
end
print("while count:", count)

-- Repeat-until loop
local n = 1
repeat
    n = n * 2
until n > 50
print("repeat n:", n)

-- Break in loop
local sum = 0
for i = 1, 100 do
    if i > 10 then
        break
    end
    sum = sum + i
end
print("break sum:", sum)
