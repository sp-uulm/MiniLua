t = {}
mt = {}

mt.__lt = function (table, other)
    return true
end

setmetatable(t, mt)

-- will call __lt(2, t) and negate the result if __le is not present
assert(t <= 2 == false)

mt.__le = nil
mt.__le = function (table, other)
    return true
end

assert(t <= 2)

------

mt = {}
mt.__lt = function (table, other)
    return table.value < other.value
end
mt.__eq = function (table, other)
    return table.value == other.value
end

t1 = {}
t1.value = 4
setmetatable(t1, mt)

t2 = {}
t2.value = 4
setmetatable(t2, mt)

t3 = {}
t3.value = 20
setmetatable(t3, mt)

assert(t1 == t1)
assert(t2 == t2)
assert(t3 == t3)

assert(t1 == t2)
assert(t1 ~= t3)
assert(t2 ~= t3)

-- assert(not (t1 < t2))
-- assert(not (t1 > t2))
assert(t1 < t3)
assert(t2 < t3)
assert(t3 > t1)
assert(t3 > t2)

assert(t1 <= t3)
assert(t2 <= t3)
assert(t3 >= t1)
assert(t3 >= t2)
