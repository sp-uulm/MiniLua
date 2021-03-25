a = 0
for i=1,6,2 do
    a = a+1
end

assert(a==3)
t={
    a = "b";
    c ;
    d=2;
    e = 2;
}

i = 0;
for k,v in next,t,nil do
    i = i + 1
end
assert(i==4);

t2 = {
    a = 2;
    b = 3;
    c = 4;
    d = 5
}

sum = 0;
for k,v in next,t2,nil do
    sum = sum + v;
end
assert(sum == 14)