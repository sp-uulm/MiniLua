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
