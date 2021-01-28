function counter()
    local i = 0
    return function()
        i = i + 1
        return i
    end
end

c1 = counter()
c2 = counter()

assert(c1() == 1)
assert(c2() == 1)
assert(c1() == 2)
assert(c2() == 2)
assert(c1() == 3)
assert(c1() == 4)
assert(c1() == 5)
assert(c2() == 3)

