x = 10
do
    local x = x
    assert(x == 10)
    x = x + 1

    do
        local x = x + 1
        assert(x == 12)
    end

    assert(x == 11)
end

assert(x == 10)
