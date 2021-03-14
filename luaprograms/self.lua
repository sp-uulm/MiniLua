

t = {}

function t:f1()
    print(self)
end

t:f1(1) -- print table 0x...

function t:f2(self)
    print(self)
end

t:f2(1) -- print 1
