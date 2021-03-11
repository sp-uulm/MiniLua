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

mt.__concat = function (table, other)
    return table.value .. other.value
end

assert(t1 .. t2 == "onetwo")
assert(t1 .. t3 == "onethree")
assert(t2 .. t3 == "twothree")
