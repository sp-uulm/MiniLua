t1 = {}
t1.value = 4
t2 = {}
t2.value = 10
t3 = {}
t3.value = 2

mt = {}
setmetatable(t1, mt)
setmetatable(t2, mt)
setmetatable(t3, mt)

-- band
mt.__band = function (table, other)
    return table.value & other.value
end

assert(t1 & t2 == 0)
assert(t1 & t3 == 0)
assert(t2 & t3 == 2)

-- bor
mt.__bor = function (table, other)
    return table.value | other.value
end

assert(t1 | t2 == 14)
assert(t1 | t3 == 6)
assert(t2 | t3 == 10)

-- bxor
mt.__bxor = function (table, other)
    return table.value ~ other.value
end

assert(t1 ~ t2 == 14)
assert(t1 ~ t3 == 6)
assert(t2 ~ t3 == 8)

-- bnot
mt.__bnot = function (table)
    return ~table.value
end

assert(~t1 == -5)
assert(~t2 == -11)
assert(~t3 == -3)

-- shl
mt.__shl = function (table, other)
    return table.value << other.value
end

assert(t1 << t2 == 4096)
assert(t1 << t3 == 16)
assert(t2 << t3 == 40)

-- shr
mt.__shr = function (table, other)
    return table.value >> other.value
end

assert(t1 >> t2 == 0)
assert(t1 >> t3 == 1)
assert(t2 >> t3 == 2)
