assert(tostring(2) == "2")
assert(math.type(2) == "integer")
assert(tostring(-15) == "-15")
assert(math.type(-15) == "integer")
assert(tostring(22e4) == "220000.0")
assert(math.type(22e4) == "float")
assert(tostring(-17e4) == "-170000.0")
assert(math.type(-17e4) == "float")
assert(tostring(2.5) == "2.5")
assert(math.type(2.5) == "float")
assert(tostring(-179.42) == "-179.42")
assert(math.type(-179.42) == "float")
assert(tostring(52) == "52")
assert(math.type(10e1) == "float")
assert(tostring(.42) == "0.42")
assert(math.type(.42) == "float")
assert(tostring(0x.4) == "0.25")
assert(math.type(0x.4) == "float")
assert(tostring(0x.4p2) == "1.0")
assert(math.type(0x.4p2) == "float")
assert(tostring(0x.4p+2) == "1.0")
assert(math.type(0x.4p+2) == "float")
assert(tostring(0x0.4p-1) == "0.125")
assert(math.type(0x0.4p-1) == "float")
assert(tostring(-0x.4p+2) == "-1.0")
assert(math.type(-0x.4p+2) == "float")
assert(tostring(  -  0x0.4p-1) == "-0.125")
assert(math.type(  -  0x0.4p-1) == "float")
assert(tostring(- 179.42e2) == "-17942.0")
assert(math.type(- 179.42e2) == "float")
assert(tostring(- .2e-2) == "-0.002")
assert(math.type(- .2e-2) == "float")
assert(tostring(3.2e+2) == "320.0")
assert(math.type(3.2e+2) == "float")


assert(tostring(0xffffffffffffff0c) == "-244")
assert(math.type(0xffffffffffffff0c) == "integer")
-- assert(tostring(0xffffffffffffffff0c) == "-244")
-- assert(math.type(0xffffffffffffffff0c) == "integer")
assert(tostring(-0xffffffffffffff0c) == "244")
assert(math.type(-0xffffffffffffff0c) == "integer")
-- assert(tostring(-0xffffffffffffffff0c) == "244")
-- assert(math.type(-0xffffffffffffffff0c) == "integer")