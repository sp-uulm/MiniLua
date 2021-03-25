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

-- add
mt.__add = function (table, other)
    return table.value + other.value
end

assert(t1 + t2 == 14)
assert(t1 + t3 == 6)
assert(t2 + t3 == 12)

-- sub
mt.__sub = function (table, other)
    return table.value - other.value
end

assert(t1 - t2 == -6)
assert(t1 - t3 == 2)
assert(t2 - t3 == 8)

-- mul
mt.__mul = function (table, other)
    return table.value * other.value
end

assert(t1 * t2 == 40)
assert(t1 * t3 == 8)
assert(t2 * t3 == 20)

-- div
mt.__div = function (table, other)
    return table.value / other.value
end

assert(t1 / t2 == 0.4)
assert(t1 / t3 == 2)
assert(t2 / t3 == 5)

-- mod
mt.__mod = function (table, other)
    return table.value % other.value
end

assert(t1 % t2 == 4)
assert(t1 % t3 == 0)
assert(t2 % t3 == 0)

-- pow
mt.__pow = function (table, other)
    return table.value ^ other.value
end

assert(t1 ^ t2 == 1048576)
assert(t1 ^ t3 == 16)
assert(t2 ^ t3 == 100)

-- unm
mt.__unm = function (table)
    return -table.value
end

assert(-t1 == -4)
assert(-t2 == -10)
assert(-t3 == -2)

-- idiv
mt.__idiv = function (table, other)
    return table.value // other.value
end

assert(t1 // t2 == 0)
assert(t1 // t3 == 2)
assert(t2 // t3 == 5)
