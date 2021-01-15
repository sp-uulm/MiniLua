global1 = 1
assert(global1 == 1)

local local1 = 1
assert(local1 == 1)

assert(local2 == nil)

do
    assert(global1 == 1)
    assert(local1 == 1)

    local local2 = 2
    assert(local2 == 2)
end

assert(global1 == 1)
assert(local2 == nil)

do
    assert(global1 == 1)
    assert(local1 == 1)

    local local3 = 3
    assert(local3 == 3)
end

assert(global1 == 1)
assert(local3 == nil)

do
    local local4
    assert(local4 == nil)

    do
        local4 = 4
        assert(local4 == 4)
    end

    assert(local4 == 4)
end
