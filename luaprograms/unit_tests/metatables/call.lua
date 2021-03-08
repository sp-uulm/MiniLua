t = {}
mt = {}
mt.__call = function(table, ...)
    return ...
end

setmetatable(t, mt)

assert(t("hi") == "hi")
