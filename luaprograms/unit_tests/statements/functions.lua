a1 = false
function f1()
    a1 = true
end
f1()
assert(a1)

a2 = false
function f2(x)
    if x then
        a2 = true
    end
end
assert(not a2)
f2(false)
f2(true)
assert(a2)

a3 = 0
function f3()
    a3 = a3 + 1
end
f3()
assert(a3 == 1)
f3()
assert(a3 == 2)
f3()
assert(a3 == 3)
f3()
assert(a3 == 4)
f3()
assert(a3 == 5)

a4 = 0
f4 = function ()
    a4 = a4 + 1
end
f4()
assert(a4 == 1)

function f5(x, y)
    return x + y
end
assert(f5(1, 1) == 2)
assert(f5(2, 4) == 6)
