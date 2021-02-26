max = 1000
i = 0

while i < max do
    i = discard_origin(i + 1)
    -- i = i + 1
end
assert(i == max)
