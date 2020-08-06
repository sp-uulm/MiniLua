# MiniLua

MiniLua is a small lua interpreter that implements additional features like source location tracking. It is currently used in and developed for the interactive_script rqt plugin.

## Building MiniLua

```
mkdir build && cd build
cmake ..
make -j4
```

## Usage of MiniLua-cli and MiniLua-gui

MiniLua-cli executes some test strings, defined in `src/cli/main.cpp`.
It currently does not support executing scripts or an interactive mode.

MiniLua-gui opens a window with an editor and a preview. The editor contains a small example program.
It highlights positions that would be changed due to forcing values but does not actually change them. The preview is currently completely non-interactive.

MiniLua-gui currently doesn't use the `eval_result_t` to get the source changes. This is deprecated and highly discouraged. The interactive_script plugin for rqt is a much better and more current example of MiniLua's usage.

# Embedding MiniLua

Both MiniLua-cli and MiniLua-gui show how libMiniLua can be used. 

## Calling C++ Functions from Lua

`gui.cpp` shows how additional functions can be registered.

## Calling Lua Functions from C++

You would need to create a `LuaFunctioncall` from the `lfunction` and then visit it using the interpreters ASTEvaluator. No idea on the details currently. I have never tried it.


