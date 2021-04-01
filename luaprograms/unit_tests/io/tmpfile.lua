file1 = io.tmpfile()
file2 = io.tmpfile()

assert(file1 ~= file2)

file1:write("abc")
file1:flush()
file1:seek("set")
assert(file1:read("a") == "abc")

file2:write("1234")
file2:flush()
file2:seek("set")
assert(file2:read("a") == "1234")
