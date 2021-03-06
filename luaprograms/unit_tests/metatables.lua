t1 = {}
mt1 = {}

assert(getmetatable(t1) == nil)

assert(setmetatable(t1, nil) == t1)
assert(getmetatable(t1) == nil)

assert(setmetatable(t1, mt1) == t1)
assert(getmetatable(t1) == mt1)

assert(setmetatable(t1, nil) == t1)
assert(getmetatable(t1) == nil)

function __index(table, index)
    return index
end

mt1 = {__index = __index}
setmetatable(t1, mt1)

assert(t1["hi"] == "hi")
assert(t1[4] == 4)
assert(t1.key == "key")
