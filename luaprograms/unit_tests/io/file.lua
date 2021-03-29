assert(io.type(25) == nil)

file = io.open("/tmp/luatest.txt", "w")
assert(io.type(file) == "file")
file:write("Hello, World!")
-- file.flush(file)
file:close()
assert(io.type(file) == "closed file")

file = io.open("/tmp/luatest.txt", "r")
assert(io.type(file) == "file")
content = file:read("a")
file:close()
assert(content == "Hello, World!")
assert(io.type(file) == "closed file")

file = io.open("/tmp/luatest.txt", "r")
-- seek to end to get file size
assert(file:seek("end") == 13)
content = file:read("a")
-- close using io.close instead of file:close
io.close(file)
assert(content == "")

-- setvbuf can't be tested except that the file still works after calling it
file = io.open("/tmp/luatest.txt", "r")
assert(file:setvbuf("no") == true)
content = file:read("a")
file:close()
assert(content == "Hello, World!")

file = io.open("/tmp/luatest.txt", "r")
assert(file:setvbuf("line") == true)
content = file:read("a")
file:close()
assert(content == "Hello, World!")

file = io.open("/tmp/luatest.txt", "r")
assert(file:setvbuf("full", 8) == true)
content = file:read("a")
file:close()
assert(content == "Hello, World!")
