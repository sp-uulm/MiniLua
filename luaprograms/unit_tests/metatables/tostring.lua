t = {}
mt = {}
mt.__tostring = function (value)
    return "__tostring"
end

setmetatable(t, mt)

assert(tostring(t) == "__tostring")
