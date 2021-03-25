t1 = {1, 2, 3}

t2 = {}
mt = {}
setmetatable(t2, mt)

mt.__index = function (table, index)
    return t1[index]
end

force(t2[1], 4)
-- EXPECT SOURCE_CHANGE 1:7 4
