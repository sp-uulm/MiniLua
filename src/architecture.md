# Architecture Overview {#architecture}

\tableofcontents

The following will give a short overview of the architecture used in the parser,
AST and Interpreter.

The library is divided into the public API (everything accessible through the
public headers in `include/MiniLua/`) and the private API (everything that is
not accessible by the user of the library). To properly separate the public and
private API we use the *PImpl technique* in the public API to hide the
implementation details. See [below](@ref pimpl).

The library is overall divided into three parts:

1. [Parser](@ref parser)
2. [Abstract Syntax Tree](@ref ast) (AST)
3. [Interpreter](@ref interpreter)

@dot
strict digraph {
    subgraph cluster {
        label = "Created by User";
        style = filled;
        color = lightgrey;

        SourceCode [label="Source Code"];
        Environment [label="Pre-Setup Environment"];
    }

    ReturnValue [label="Return Value"];

    SourceCode -> Parser;
    Parser -> AST;
    Environment -> Interpreter;
    AST -> Interpreter;
    Interpreter -> SourceChanges;
    Interpreter -> ReturnValue;
    SourceChanges -> SourceCode [label="Apply to Code", style=dashed];
}
@enddot

## Parser {#parser}

This library uses [Tree-Sitter](https://tree-sitter.github.io/tree-sitter/) as
the parser. Tree-Sitter is only a parser generator library and we use
[our own grammar definition](https://github.com/sp-uulm/tree-sitter-lua) for Lua
which is a fork of [Azganoth/tree-sitter-lua](https://github.com/Azganoth/tree-sitter-lua).

The tree-sitter parser takes the source code provided by the user and creates a
tree of nodes. We then convert this tree into our own abstract syntax tree.

## Abstract Syntax Tree {#ast}

The abstract syntax tree (AST) makes it a bit more convenient for the
interpreter to navigate through the tree. The AST also takes care of
*desugaring* some of the syntax. Desugaring converts more complicated syntax to
simpler syntax. E.g. for loops get desugared into while loops and method
definitions and calls get desugared into normal function definitions and calls.

## Interpreter {#interpreter}

The interpreter walks through the AST, generates [Values](@ref minilua::Value)
for the literals and executes the code. It also takes care of handling and
combining the [SourceChanges](@ref minilua::SourceChange).

For (almost) every AST node there is one `visit_*` method in the internal
[Interpreter](@ref minilua::details::Interpreter). The methods take the AST node
and the current environment (in form of the internal
[Env](@ref minilua::Env)). We need this environment to correctly scope
local variables. We create a new environment whenever there is a new block/scope
and the local variable declarations are then only added to this environment.
When we leave the block/scope we switch back to the outer environment.

## Allocator

The interpreter keeps track of all [Values](@ref minilua::Value) and the
[Environments](@ref minilua::Environment) that are created. We do this to
prevent memory leaks.

\note In the future it would also be possible to implement a garbage collector.

[MemoryAllocator](@ref minilua::MemoryAllocator) is the class that handles all
allocation and keeps track of the values. Actually the allocator only keeps
track of the tables because they are the only type that can create reference
cycles, which would lead to a memory leak. Cycles can happen in the following
ways:

- Direct or indirect cycles in tables. I.e.
  @code{.lua}
  t1 = {}
  t2 = {table1 = t1}
  t1["table2"] = t2
  @endcode
- Functions capture their environment but are also stored in the environment.
  Which also creates a cycle.

## Implementation

The following sections describe some implementation details/techniques.

### PImpl Technique {#pimpl}

Also see: https://en.cppreference.com/w/cpp/language/pimpl

With the PImpl technique we hide all behaviour using a (private) nested forward
declaration of a struct or class. This has two important uses:

1. hiding the implementation details from the user
2. breaking up cycles of incomplete types
   (this works because the cyclic reference are now in the cpp instead of the header)

For this to work classes using the PImpl technique can't *use* the private nested
forward declaration. This means we have to declare but not define the methods
in the header. This includes special member functions like copy-constructor and
destructor. Then these methods have to be implemented in the cpp file and the
only difference is basically instead of using `this->` they have to use
`this->impl->` or just `impl->`.

A class with the PImpl technique usually looks like this:

```cpp
// in the header
class Something {
  struct Impl;
  owning_ptr<Impl> impl; // any pointer type is ok here

public:
  // normal constructor declarations
  Something(const Something&);
  Something(Something&&) noexcept;
  Something& operator=(const Something& other);
  Something& operator=(Something&& other);
  friend void swap(Something& self, Something& other);
};

// in the cpp
struct Something::Impl {
  // fields you would have put in class Something
};
Something::Something(const Something&) = default;
Something(Something&&) noexcept = default;
Something& operator=(const Something& other) = default;
Something& operator=(Something&& other) = default;
void swap(Something& self, Something& other) {
  std::swap(self.impl, other.impl);
}
```

If the nested `struct Impl` is not default constructible you have to provide
the implementation of the copy-/move-constructors and -operators manually and
can't use `= default`.

[owning_ptr<T>](@ref minilua::owning_ptr) was chosen for the pointer type
because it behaves like a normal `T` (but lives on the heap). It's move, copy
and lifetime semantics are identical to the one of `T`.

It is also possible to choose another pointer type like `std::unique_ptr` or
`std::shared_ptr`. You should probably avoid using raw pointer `T*` because
lifetime management is hard with raw pointers.

