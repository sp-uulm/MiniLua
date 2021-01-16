i = 10
j = 0
while i > 0 do
    i = i - 1
    j = j + 1
end
assert(i == 0)
assert(j == 10)

while false do
    assert(false)
end

x = 0
while x < 10 do
    local y = 0

    while true do
        y = y + 1
        if y == 5 then
            break
        end
    end

    x = x + y
end
assert(x == 10)
