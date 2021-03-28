assert(io.type(25) == nil)

file = io.open("/tmp/luatest.txt", "w")
assert(io.type(file) == "file")
file.write(file, "Hello, World!")
-- file.flush(file)
file.close(file)
assert(io.type(file) == "closed file")

file = io.open("/tmp/luatest.txt", "r")
assert(io.type(file) == "file")
content = file.read(file, "a")
file.close(file)
assert(content == "Hello, World!")
assert(io.type(file) == "closed file")

file = io.open("/tmp/luatest.txt", "r")
-- seek to end to get file size
assert(file.seek(file, "end") == 13)
content = file.read(file, "a")
-- close using io.close instead of file:close
io.close(file)
assert(content == "")
