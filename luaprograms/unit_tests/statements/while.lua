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
