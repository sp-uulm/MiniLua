# MiniLua

![MiniLua](https://github.com/sp-uulm/MiniLua/workflows/MiniLua/badge.svg)
[![codecov](https://codecov.io/gh/sp-uulm/MiniLua/branch/master/graph/badge.svg?token=AM4H5B338A)](https://codecov.io/gh/sp-uulm/MiniLua)

MiniLua is a small lua interpreter that implements additional features like source location tracking. It is currently used in and developed for the interactive_script rqt plugin. The targeted lua-version is Lua 5.3

Also see: [Documentation](https://sp-uulm.github.io/MiniLua/)

## Building MiniLua

Make sure you cloned the repository with submodules (e.g. `git clone --recurse-submodules git@github.com:sp-uulm/MiniLua.git`) or if you already cloned the repository you need to fetch the submodules (e.g. `git submodule update --init --recursive`).

Now you can use it like any other CMake project:

```sh
mkdir build
cd build
cmake ..
make
```

Alternatively you can use the included scripts:

```sh
./scripts/setup_build.sh # will forward arguments to cmake
./scripts/build.sh # will forward arguments to make
```

### Generating Documentation

The documentation uses [Doxygen](https://www.doxygen.nl/index.html) and the
C++ Doxygen Theme of [m.css](https://mcss.mosra.cz/). You need the
`doxygen` command installed and you Python 3.6 as well as Jinja2 and Pygments
installed:

```sh
# You may need sudo here
pip3 install jinja2 Pygments
```

To build the documentation you can simply run:

```sh
./scripts/docs.sh
```

You can now open `build/docs/html/index.html` in your browser and you should see
the main page of the documentation.

## Using MiniLua

For more information see the [documentation](https://sp-uulm.github.io/MiniLua/)

### Example Applications

There are two included example applications (possibly out of date). They can be found in the `examples` directory.

- MiniLua-cli: Executes some test strings.
- MiniLua-gui: Opens a window with an editor and a preview. The editor highlights positions that would be changed due to forcing the values. The preview is completely non-interactive.

For a more up-to-date example take a look at the *interactive_script plugin for rqt*.

