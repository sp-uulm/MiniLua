t1 = {}
assert(#t1 == 0)

t6 = {42, 43, 44}
assert(#t6 == 3)

t2 = {key = 42}
assert(#t2 == 0)
assert(t2.key == 42)
assert(t2["key"] == 42)

t3 = {["key"] = 42}
assert(#t3 == 0)
assert(t3.key == 42)
assert(t3["key"] == 42)

t4 = {key1 = 42, key2 = 43}
assert(#t4 == 0)
assert(t4.key1 == 42)
assert(t4["key1"] == 42)
assert(t4.key2 == 43)
assert(t4["key2"] == 43)

t5 = {key1 = 42; key2 = 43}
assert(#t5 == 0)
assert(t5.key1 == 42)
assert(t5["key1"] == 42)
assert(t5.key2 == 43)
assert(t5["key2"] == 43)

t7 = {key = {sub_key = 42}}
assert(#t7 == 0)
assert(#t7.key == 0)
assert(#t7["key"] == 0)
assert(t7.key.sub_key ==  42)
assert(t7["key"]["sub_key"] ==  42)

t8 = {[24] = 14, [12.4] = 22}
assert(#t8 == 0)
assert(t8[24] == 14)
assert(t8[12.4] == 22)

t9 = {22}
assert(#t9 == 1)

t10 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}
assert(#t10 == 12)

t11 = {1, 2, 3, 4, key = true, key2 = false}
assert(#t11 == 4)

t12 = {1, key = 2, [3] = 3}
assert(#t12 == 1)

t13 = {[1] = 1, [2] = 2, [4] = 4}
-- both are correct according to the definition in the lua spec:
--   (border == 0 or t[border] ~= nil) and t[border + 1] == nil
assert(#t13 == 2 or #t13 == 4)
