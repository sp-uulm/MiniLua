DOXYFILE = "Doxyfile-mcss"

MAIN_PROJECT_URL = 'https://sp-uulm.github.io/MiniLua/'

LINKS_NAVBAR1 = [
    (None, 'pages', []),
    (None, 'namespaces', [
        (None, 'namespaceminilua'),
        (None, 'namespacets'),
    ]),
]

LINKS_NAVBAR2 = [
    ("Classes", 'annotated', []),
    ('<a href="https://github.com/sp-uulm/MiniLua/">GitHub</a>', [])
]

M_CODE_FILTERS_PRE = {
    "Lua": lambda code: "-- Lua Code\n" + code
}

