# MiniLua {#mainpage}

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

# Usage Example

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
