#ifndef MINILUA_INTERNAL_ENV
#define MINILUA_INTERNAL_ENV

#include <unordered_map>

#include <MiniLua/values.hpp>

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
 * Usage: When a new block is started (e.g. the body of a for loop) the local
 * environment shoule be copied so it can be extended with new local variables
 * without changing the old environment.
 *
 * TODO potential memory and performance improvements by nesting multiple local
 * environments (lower memory footprint, faster creation, slower lookup)
 *
 * TODO maybe replace the shared_ptr<Value> by something else we also use in the
 * interpreter
 */
using LocalEnv = std::unordered_map<std::string, std::shared_ptr<Value>>;

/**
 * Environment for use in the interpreter.
 */
class Env {
    MemoryAllocator* allocator;
    Table _global;
    LocalEnv _local;
    std::optional<Vallist> varargs;

    // iostreams
    std::istream* in;
    std::ostream* out;
    std::ostream* err;

public:
    Env();
    Env(MemoryAllocator* allocator);

    explicit operator Environment() const;

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
     * Get a locally defined value, if it exists.
     */
    auto get_local(const std::string&) -> std::optional<Value>;

    /**
     * Check if a local variable with the name `name` is declared.
     */
    auto is_local(const std::string&) -> bool;

    /**
     * Sets the value of a global variable.
     */
    void set_global(const std::string&, Value);

    /**
     * Gets the value of a local variable or Nil if it was not defined.
     */
    auto get_global(const std::string&) -> Value;

    /**
     * Set a variable named `name` to `value`.
     *
     * If `name` is declared as a local variable the value of that variable will
     * be changed. Otherwise the value of the global variable named `name` will
     * be changed/set.
     */
    void set_var(const std::string& name, Value value);

    /**
     * Get the value of a variable `name` or Nil if it is not set.
     */
    auto get_var(const std::string& name) -> Value;

    /**
     * Setter and getter for the varargs of the immediately enclosing varargs
     * function.
     */
    void set_varargs(std::optional<Vallist> vallist);
    auto get_varargs() const -> std::optional<Vallist>;

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

auto operator<<(std::ostream&, const Env&) -> std::ostream&;

struct Environment::Impl {
    Env inner;

    Impl(Env env);
    Impl(MemoryAllocator* allocator);
};

}; // namespace minilua

#endif
