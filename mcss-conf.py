DOXYFILE = "Doxyfile-mcss"

M_CODE_FILTERS_PRE = {
    "Lua": lambda code: "-- Lua Code\n" + code
}
