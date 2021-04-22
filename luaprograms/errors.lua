function func1()
  func2()
end

function func2()
  func3()
end

function func3()
  error("some error")
end

func1()

-- t1 = {}
-- t2 = {}
-- mt = {}
-- mt.__add = function (self, other)
--   local t = {}
--   setmetatable(t, mt)
--   return t
-- end
-- mt.__call = function (self)
--   error("err")
-- end
-- setmetatable(t1, mt)
-- setmetatable(t2, mt)
-- (t1 + t2)()
