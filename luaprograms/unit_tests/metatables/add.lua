t1 = {1, 2, 3}
t2 = {2, 3}

function __add(x, y)
    return #x + #y
end

mt = {}
mt.__add = __add

setmetatable(t1, mt)

assert(t1 + t2 == 5)
assert(t2 + t1 == 5)
assert(t1 + t1 == 6)
