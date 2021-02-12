DOXYFILE = "Doxyfile-mcss"

MAIN_PROJECT_URL = 'https://sp-uulm.github.io/MiniLua/'

M_CODE_FILTERS_PRE = {
    "Lua": lambda code: "-- Lua Code\n" + code
}

