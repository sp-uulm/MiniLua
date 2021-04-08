assert(1 + 4 == 5)
assert(true and 7 == 7)
assert(true and false == false)
assert(7 or false == 7)
assert("hi " .. "world" == "hi world")
assert(3 - 5.2 == -2.2)
assert(7 * 1.5 == 10.5)
assert(40 / 8 == 5)
assert(42 // 8 == 5)
assert(3 ^ 4 == 81)
assert(17 % 4 == 1)

-- bit and
assert(0xf3 & 0x42 == 0x42)
assert(math.type(0xf3 & 0x42) == "integer")
assert(0xf3.0 & 0x42.0 == 0x42)
assert(math.type(0xf3.0 & 0x42.0) == "integer")
-- bit or
assert(0x33 | 0x44 == 0x77)
assert(math.type(0x33 | 0x44) == "integer")
assert(0x33.0 | 0x44.0 == 0x77)
assert(math.type(0x33.0 | 0x44.0) == "integer")
-- bit xor
assert(0xf3 ~ 0x4f == 0xBC)
assert(math.type(0xf3 ~ 0x4f) == "integer")
assert(0xf3.0 ~ 0x4f.0 == 0xBC)
assert(math.type(0xf3.0 ~ 0x4f.0) == "integer")
-- bit shift right
assert(0xf3 >> 2 == 0x3C)
assert(math.type(0xf3 >> 2) == "integer")
assert(0xf3.0 >> 2.0 == 0x3C)
assert(math.type(0xf3.0 >> 2.0) == "integer")
-- bit shift left
assert(0xf3 << 2 == 0x3CC)
assert(math.type(0xf3 << 2) == "integer")
assert(0xf3.0 << 2.0 == 0x3CC)
assert(math.type(0xf3.0 << 2.0) == "integer")
-- unary bit xor
assert(~0xf3 == 0xffffffffffffff0c)
assert(math.type(~0xf3) == "integer")

assert(22 ~= 21)
assert(22 >= 21)
assert(22 > 21)
assert(17 <= 21)
assert(17 < 21)

assert((44 / 2) == 22)
assert(22 == (44 / 2))

assert((3 ^ 5 * 4) == (4 * 3 ^ 5))

assert(5 ^ ((6 - 2) / 4) == 5)

assert(5 ^ (6 - 2) / 4 == 156.25) -- 5^4 = 625 -> 625 / 4 = 156.25