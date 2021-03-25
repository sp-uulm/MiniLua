file = io.open("/tmp/luatest.txt", "w")
file.write(file, "Hello, World!")
-- file.flush(file)
file.close(file)

file = io.open("/tmp/luatest.txt", "r")
content = file.read(file, "a")
file.close(file)
assert(content == "Hello, World!")
