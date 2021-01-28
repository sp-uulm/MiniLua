i = 0

repeat
    i = i + 1
    local x = i
until x == 10

assert(i == 10)

repeat
    i = i + 1
    local x = i

    if x == 15 then
        break
    end
until x == 20

assert(i == 15)
