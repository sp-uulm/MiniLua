t1 = {1, 2, 3}
mt = {}
setmetatable(t1, mt)
mt.__add = function (table, rhs)
    return table[1] + rhs
end

force(t1 + 1, 7)
-- EXPECT SOURCE_CHANGE 1:7 6
-- EXPECT SOURCE_CHANGE 8:12 6
