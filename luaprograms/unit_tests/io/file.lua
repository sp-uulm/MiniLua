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

-- setvbuf can't be tested except that the file still works after calling it
file = io.open("/tmp/luatest.txt", "r")
assert(file.setvbuf(file, "no") == true)
content = file.read(file, "a")
file.close(file)
assert(content == "Hello, World!")

file = io.open("/tmp/luatest.txt", "r")
assert(file.setvbuf(file, "line") == true)
content = file.read(file, "a")
file.close(file)
assert(content == "Hello, World!")

file = io.open("/tmp/luatest.txt", "r")
assert(file.setvbuf(file, "full", 8) == true)
content = file.read(file, "a")
file.close(file)
assert(content == "Hello, World!")
