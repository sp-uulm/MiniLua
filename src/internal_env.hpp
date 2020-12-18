#ifndef MINILUA_INTERNAL_ENV
#define MINILUA_INTERNAL_ENV

#include <unordered_map>

#include "MiniLua/values.hpp"

namespace minilua {

/**
 * Environment for use in the interpreter.
 */
class Env {
    Table table;
    // iostreams
    std::istream* in;
    std::ostream* out;
    std::ostream* err;

public:
    Env();

    /**
     * Returns the table for the global environment.
     */
    auto global() -> Table&;
    [[nodiscard]] auto global() const -> const Table&;

    /**
     * Sets stdin/out/err stream to use in lua code.
     *
     * NOTE: The default are c++'s cin, cout and cerr.
     */
    void set_stdin(std::istream*);
    void set_stdout(std::ostream*);
    void set_stderr(std::ostream*);

    /**
     * Get the configured stdin/out/err stream.
     */
    auto get_stdin() -> std::istream*;
    auto get_stdout() -> std::ostream*;
    auto get_stderr() -> std::ostream*;
};

struct Environment::Impl {
    Env inner;
};

}; // namespace minilua

#endif
