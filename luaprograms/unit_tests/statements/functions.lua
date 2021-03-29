a1 = false
function f1()
    a1 = true
end
f1()
assert(a1)

a2 = false
function f2(x)
    if x then
        a2 = true
    end
end
assert(not a2)
f2(false)
f2(true)
assert(a2)

a3 = 0
function f3()
    a3 = a3 + 1
end
f3()
assert(a3 == 1)
f3()
assert(a3 == 2)
f3()
assert(a3 == 3)
f3()
assert(a3 == 4)
f3()
assert(a3 == 5)

a4 = 0
f4 = function ()
    a4 = a4 + 1
end
f4()
assert(a4 == 1)

function f5(x, y)
    return x + y
end
assert(f5(1, 1) == 2)
assert(f5(2, 4) == 6)

-- varargs
a5 = 0
function f5 (...)
    a5 = #{...}
end
f5(1, 2, 3, 4)
assert(a5 == 4)
f5(1)
assert(a5 == 1)

function returns_varargs()
    return 1, 2, 3
end
f5(1, 2, 3, returns_varargs())
assert(a5 == 6)

function f6(s)
    assert(type(s) == "string")
end
f6"hi"
f6'hi'
f6[[hi]]
(f6("hi"))
;(f6("hi"))

function f7(t)
    assert(t.key == 42)
end
f7{key = 42}

t2 = {
    a = 2;
    b = 3;
    c = 4;
    d = 5;
    t3 = {
        func = function (a)
            return a*7
        end;
        a = 144;
        b = 13;
    }
}

function t2:sum ()
    return self.a+self.b+self.c+self.d
end

sum = t2.sum(t2)
assert(sum == 14)
sum2 = t2:sum()
assert(sum == sum2)


res = t2.t3.func(6)
assert(res == 42)

function t2.t3:foo()
    return self.a % self.b --144 mod 13 = 1
end

res2 = t2.t3:foo();
assert(res2 == 1)