assert(io.input() == io.stdin)
assert(io.output() == io.stdout)

file1 = io.open("/tmp/test.txt", "w")
file1:write("Test Line")
file1:close()

file1 = io.open("/tmp/test.txt", "r")
io.input(file1)
assert(io.input() == file1)

file2 = io.open("/tmp/test2.txt", "w")
io.output(file2)
assert(io.output() == file2)

io.input("/tmp/test.txt")
assert(io.input():read("a") == "Test Line")

io.output("/tmp/test2.txt")
assert(io.output():write("Test2 Line"))

io.close()
assert(io.type(io.output()) == "closed file")

file = io.open("/tmp/test2.txt", "r")
assert(file:read("a") == "Test2 Line")
file:close()

-- line_func = io.lines("/tmp/test2.txt")
-- assert(line_func() == "Test2 Line")
-- assert(line_func() == nil)
-- assert(not pcall(function () line_func() end))
