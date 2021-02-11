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

## Origin Tracking and Source Changes

This Lua interpreter tracks the [Origin](@ref minilua::Origin) of all (or most)
[Values](@ref minilua::Value). The following Lua code

```lua
x = 10
y = 20
return x^2 + y^2
```

returns the value 500 and that value has the following origin:

\startuml
title
{{wbs
* BinaryOrigin
** lhs: BinaryOrigin
*** lhs: LiteralOrigin
**** value: 10
**** line: 1
*** rhs: LiteralOrigin
**** value: 2
**** line: 3
** rhs: BinaryOrigin
*** lhs: LiteralOrigin
**** value: 20
**** line: 2
*** rhs: LiteralOrigin
**** value: 2
**** line: 3
}}
end title
\enduml

This origin hierarchy allows the interpreter to generate
[SourceChanges](@ref minilua::SourceChange) if you want to [force](@ref minilua::Value::force)
a value to a have a different value. I.e. here we force the value of the expression
`x^2 + y^2` (which is 500) to `400`.

```lua
x = 10
y = 20
z_squared = x^2 + y^2
force(z_squared, 400)
```

This code would generate the following source changes:

\startuml
title
{{wbs
* SourceChangeAlternative
** SourceChangeAlternative
*** SourceChange replace "10" on line 1 with "0"
*** SourceChange replace the first "2" on line 3 with "-9223372036854775808"
** SourceChangeAlternative
*** SourceChange replace "20" on line 2 with "17.3205"
*** SourceChange replace the second "2" on line 3 with "1.90397"
}}
end title
\enduml

Here a [`SourceChangeAlternative`](@ref minilua::SourceChangeAlternative) means
that exactly one of the *children* should be applied. In this case (because
multiple `SourceChangeAlternative`s are nested) we could flatten the hierarchy.
If we apply any of the source changes we will have forced the old value of
`z_squared` (which was 500) to the new value 400.

Note that the second source change (*replace the first "2" on line 3 with
"-9223372036854775808"*) was generated due to *floating point underflow* and while
it is correct and would yield the wanted result is probably not what you want to
apply.

See also: \ref generating-custom-sourcechanges when writing native functions.

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

### Creating Values in Native Functions

You can create most [Values](@ref minilua::Value) (except Tables) directly
through the corresponding constructor. If you want to create tables you have to
call `CallContext::make_table()`. This will ensure that the table uses the same
allocator as all other values in the interpreter. See \ref allocator.

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

## Allocator

\todo allocator

