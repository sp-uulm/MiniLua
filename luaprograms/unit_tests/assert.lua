assert(true)

assert(not pcall(function () assert(false, "message") end))
