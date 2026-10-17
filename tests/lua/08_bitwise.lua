-- Bitwise AND
local a = 0xFF00
local b = 0x0FF0
print("band:", a & b)

-- Bitwise OR
print("bor:", a | b)

-- Bitwise XOR
print("bxor:", a ~ b)

-- Bitwise NOT
print("bnot:", ~0x00FF & 0xFFFF)

-- Left shift
local val = 1
print("shl:", val << 8)

-- Right shift
local big = 256
print("shr:", big >> 4)

-- Combined bitwise operations
local flags = 0
flags = flags | 1       -- set bit 0
flags = flags | 4       -- set bit 2
print("flags:", flags)
print("test bit 0:", (flags & 1) ~= 0)
print("test bit 1:", (flags & 2) ~= 0)
print("test bit 2:", (flags & 4) ~= 0)

-- Clear a bit
flags = flags & ~1
print("after clear bit 0:", flags)
