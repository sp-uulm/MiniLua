# MiniLua

[![Build Status](https://travis-ci.com/sp-uulm/MiniLua.svg?branch=master)](https://travis-ci.com/sp-uulm/MiniLua)

MiniLua is a small lua interpreter that implements additional features like source location tracking. It is currently used in and developed for the interactive_script rqt plugin.

Also see: [Dokumentation](https://sp-uulm.github.io/MiniLua/)

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

## Using MiniLua

### Example Applications

There are two included example applications (possibly out of date). They can be found in the `examples` directory.

- MiniLua-cli: Executes some test strings.
- MiniLua-gui: Opens a window with an editor and a preview. The editor highlights positions that would be changed due to forcing the values. The preview is completely non-interactive.

For a more up-to-date example take a look at the *interactive_script plugin for rqt*.

### Embedding MiniLua

**TODO**

### Calling C++ Functions from Lua

**TODO**

`gui.cpp` shows how additional functions can be registered.

### Calling Lua Functions from C++

**TODO**

You would need to create a `LuaFunctioncall` from the `lfunction` and then visit it using the interpreters ASTEvaluator. No idea on the details currently. I have never tried it.


