# MiniLua {#mainpage}

\tableofcontents

MiniLua is a lua interpreter with source value tracking.

\startuml
package "Public API" {
    [Interpreter] -- [Environment (Public)]
    [Environment (Public)] -- [Value]
}

package "Private API" {
    [AST] -- [Interpreter (Intern)]
    [Environment (Intern)] -- [Interpreter (Intern)]
    [Environment (Intern)] -- [Environment (Public)]
    [Interpreter (Intern)] -- [Value]

    package "Tree-Sitter" {
        [Parser] -- [Interpreter (Intern)]
        [Tree] -- [AST]
    }
}
\enduml

## Usage Example

```{.cpp}
minilua::Interpreter interpreter;
interpreter.environment().add_default_stdlib();

// parse the program
if (!interpreter.parse("x_coord = 10; forceValue(x_coord, 25)")) {
    // parser failed
}

// run the program
minilua::EvalResult result = interpreter.evaluate();

// (optionally) apply a source change
if (result.source_change) {
    interpreter.apply_source_changes(
        result.source_change.value().collect_first_alternative());
}

// source code is now:
//
// xcoord = 10; forceValue(c_coord, 25)

// re-run
```

## Guide for Writing Native Functions

You can write your own C++ functions that are callable from Lua. These functions
need to be compatible with one of the following signatures:

```cpp
auto (minilua::CallContext) -> minilua::CallResult;
auto (minilua::CallContext) -> minilua::Value;
void (minilua::CallContext);
```

That means the return type has to be convertible to `minilua::CallResult` or
`minilua::Value` or `void` (which equivalent to `nil` in lua) and the function
needs exactly one arguments of type `minilua::CallContext` or `const
minilua::CallContext&` (the latter is preferred because it avoids a copy/move
and offer the same functionality).

You can use the `minilua::CallContext` to get access to the actual arguments
and the global environment and to call other (Lua) functions.

### Using Arguments

Examples for implementing a function that adds two values:

\snippet writing_functions.cpp Using CallResult

[CallContext::arguments().get(n)](@ref minilua::Vallist) returns the *n-th*
argument (starting at 0). If that argument wasn't provided the function returns
`minilua::Nil`. Note that you can't actually differentiate between the case
where the user did not provide an argument and the user explicitly providing
`nil`.

A more convenient way to write the same function is to directly return a `Value`:

\snippet writing_functions.cpp Using Value

\warning You have to be a bit careful when returning Values (either directly or
through a CallResult) that came from the CallContext (either arguments or from
the environment). You should think carefully if you need to remove the origin
(see \ref working-with-values). This is especially the case if you use any kind
of control flow in you function (e.g. `if`, `for`, etc.). This is important to
not break the source change mechanism.
\warning
See \ref generating-custom-sourcechanges for how to write a more complicated
function that is reversible.

\note
The above two functions are equivalent to the following Lua function:
```lua
function add(arg1, arg2)
    return arg1 + arg2
end
```

### Using the Global Environment

You can also access and modify the global environment. (Native functions do not
have access to a local environment because they were not created in one.)

\snippet writing_functions.cpp Using the global environment

\note
The above is equivalent to the following Lua function:
```lua
function add_to_global_env(arg)
    global_var = global_var + arg
end
```

If you want to create a table inside of a native function you also have to use
the CallContext so that the table uses the same allocator as all other tables
generated in the interpreter.

\snippet writing_functions.cpp Creating a table

\note
The above is equivalent to the following Lua function:
```lua
function create_a_table(key, value)
    local table = {}
    table[arg1] = arg2
    return table
end
```

### Generating Custom SourceChanges {#generating-custom-sourcechanges}

\todo generating source changes (aka allowing a function to be reversible)

## Working with Values {#working-with-values}

\todo How to work with values, operators, visit, source change tracking, origin

## Working with SourceChanges {#source-changes}

\todo how source changes work and how to use them

