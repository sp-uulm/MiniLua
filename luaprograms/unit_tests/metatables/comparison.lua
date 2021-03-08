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
