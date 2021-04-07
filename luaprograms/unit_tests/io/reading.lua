file = io.open("/tmp/luatest.txt", "w")
file:write(1, "\n", 2, "\n", 3, "\n", 4)
file:close()

file = io.open("/tmp/luatest.txt", "r")
assert(file:read("n") == 1);
assert(file:read("n") == 2);
assert(file:read("n") == 3);
assert(file:read("n") == 4);

-- jump back to the start of the file
assert(file:seek("set") == 0)

a, b, c, d = file:read("n", "n", "n", "n")
assert(a == 1)
assert(b == 2)
assert(c == 3)
assert(d == 4)

assert(file:seek("set") == 0)
a, b, c, d = file:read("l", "l", "l", "l")
assert(a == "1")
assert(b == "2")
assert(c == "3")
assert(d == "4")

assert(file:seek("set") == 0)
a, b, c, d = file:read("L", "L", "L", "L")
assert(a == "1\n")
assert(b == "2\n")
assert(c == "3\n")
assert(d == "4")

assert(file:seek("set") == 0)
a, b, c, d = file:read(2, 2, 2, 1)
assert(a == "1\n")
assert(b == "2\n")
assert(c == "3\n")
assert(d == "4")

assert(file:seek("set") == 0)
assert(file:read("a") == "1\n2\n3\n4")

assert(file:seek("set") == 0)
line_func = file:lines()
assert(line_func() == "1")
assert(line_func() == "2")
assert(line_func() == "3")
assert(line_func() == "4")

assert(file:seek("set") == 0)
line_func = file:lines("n")
assert(line_func() == 1)
assert(line_func() == 2)
assert(line_func() == 3)
assert(line_func() == 4)
file:close()

-- io.lines
line_func = io.lines("/tmp/luatest.txt", "n")
assert(line_func() == 1)
assert(line_func() == 2)
assert(line_func() == 3)
assert(line_func() == 4)

-- other number formats
file = io.open("/tmp/luatest.txt", "w")
file:write(1.25, "\n", 0.00002, "\n0x24\n3.4e2\n-25\n0x24x\nzzz")
file:close()

file = io.open("/tmp/luatest.txt", "r")
assert(file:read("n") == 1.25)
assert(file:read("n") == 0.00002)
assert(file:read("n") == 0x24)
assert(file:read("n") == 3.4e2)
assert(file:read("n") == -25)
assert(file:read("n") == nil)
assert(file:read("a") == "\n0x24x\nzzz")
current = file:seek("cur")
assert(file:read(1) == nil)
assert(file:seek("cur") == current)
file:close()
