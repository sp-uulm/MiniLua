t1 = {}
assert(#t1 == 0)

t6 = {42, 43, 44}
assert(#t6 == 3)

t2 = {key = 42}
assert(#t2 == 1)
assert(t2.key == 42)
assert(t2["key"] == 42)

t3 = {["key"] = 42}
assert(#t3 == 1)
assert(t3.key == 42)
assert(t3["key"] == 42)

t4 = {key1 = 42, key2 = 43}
assert(#t4 == 2)
assert(t4.key1 == 42)
assert(t4["key1"] == 42)
assert(t4.key2 == 43)
assert(t4["key2"] == 43)

t5 = {key1 = 42; key2 = 43}
assert(#t5 == 2)
assert(t5.key1 == 42)
assert(t5["key1"] == 42)
assert(t5.key2 == 43)
assert(t5["key2"] == 43)

t7 = {key = {sub_key = 42}}
assert(#t7 == 1)
assert(#t7.key == 1)
assert(#t7["key"] == 1)
assert(t7.key.sub_key ==  42)
assert(t7["key"]["sub_key"] ==  42)

t8 = {[24] = 14, [12.4] = 22}
assert(#t8 == 2)
assert(t8[24] == 14)
assert(t8[12.4] == 22)

