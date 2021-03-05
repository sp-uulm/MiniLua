t1 = {}
mt1 = {}

assert(getmetatable(t1) == nil)

assert(setmetatable(t1, nil) == t1)
assert(getmetatable(t1) == nil)

assert(setmetatable(t1, mt1) == t1)
assert(getmetatable(t1) == mt1)

assert(setmetatable(t1, nil) == t1)
assert(getmetatable(t1) == nil)
