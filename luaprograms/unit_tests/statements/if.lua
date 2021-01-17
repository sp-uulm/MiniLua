b1 = false
if true then
    assert(true)
    b1 = true
end
assert(b1)

b2 = true
if false then
    assert(false)
    b2 = false
end
assert(b2)

b3 = nil
if true then
    assert(true)
    b3 = true
else
    assert(false)
    b3 = false
end
assert(b3)

b4 = nil
if #"hi" > 2 then
    assert(false)
    b4 = false
else
    assert(true)
    b4 = true
end
assert(b4)

b5 = nil
if 1 > 2 then
    assert(false)
    b5 = false
elseif 3 > 4 then
    assert(false)
    b5 = false
else
    assert(true)
    b5 = true
end
assert(b5)

b6 = nil
if 7 < 23 then
    assert(true)
    b6 = true

    b7 = false
    if true then
        assert(true)
        b7 = true
    end
    assert(b7)
else
    assert(false)
    b6 = false
end
assert(b6)

b8 = nil
if 1 > 2 then
    assert(false)
    b8 = false
elseif true then
    assert(true)
    b8 = true
elseif 20 > 7 then
    assert(false)
    b8 = false
else
    assert(false)
    b8 = false
end
assert(b8)
