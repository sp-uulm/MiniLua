t1 = {}
t1.value = "one"
t2 = {}
t2.value = "two"
t3 = {}
t3.value = "three"

mt = {}
setmetatable(t1, mt)
setmetatable(t2, mt)
setmetatable(t3, mt)

mt.__len = function (table)
    return #table.value
end

assert(#t1 == 3)
assert(#t2 == 3)
assert(#t3 == 5)
