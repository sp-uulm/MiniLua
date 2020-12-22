#ifndef MINILUA_INTERNAL_ENV
#define MINILUA_INTERNAL_ENV

#include <unordered_map>

#include "MiniLua/values.hpp"

namespace minilua {

/**
 * Type used for the local environment.
 *
 * This is different from the global environment because it is handled
 * differently when capturing it in functions.
 *
 * This is also not a Table because we need to copy it when capturing local
 * variables for a function. Using a Table would cause wrong behaviour in these
 * cases. E.g. variables declared after the capturing function should not be
 * accessible in the function.
 *
 * shared_ptr is used for the value because we need to be able to assign to the
 * variable.
 *
 * TODO potential memory and performance improvements by nesting multiple local
 * environments
 *
 * TODO maybe replace the shared_ptr<Value> by something else we also use in the
 * interpreter
 */
using LocalEnv = std::unordered_map<std::string, std::shared_ptr<Value>>;

/**
 * Environment for use in the interpreter.
 */
class Env {
    Table _global;
    LocalEnv _local;

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
     * Returns the table for the local environment.
     *
     * NOTE: Use the helper methods to manipulate the local env.
     */
    auto local() -> LocalEnv&;
    [[nodiscard]] auto local() const -> const LocalEnv&;

    /**
     * Declares (or redeclares) a local variable.
     */
    void declare_local(const std::string&);

    /**
     * Sets the value of a local variable and declares it, if it is not already
     * declared.
     */
    void set_local(const std::string&, Value);

    /**
     * Get a locally defined value.
     */
    auto get_local(const std::string&) -> std::optional<Value>;

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
