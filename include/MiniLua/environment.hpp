#ifndef MINILUA_ENVIRONMENT_H
#define MINILUA_ENVIRONMENT_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "allocator.hpp"
#include "utils.hpp"
#include "values.hpp"

namespace minilua {

class Value;

/**
 * Represents the global environment/configuration for the 'Interpreter'.
 *
 * This contains things like global variables (including functions), etc.
 *
 * The default constructor initializes an empty environment with the standard
 * c++ I/O streams (std::cin, etc).
 */
class Environment {
public:
    struct Impl;

private:
    owning_ptr<Impl> impl;

public:
    Environment();
    Environment(MemoryAllocator* allocator);
    explicit Environment(Impl);
    Environment(const Environment&);
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    Environment(Environment&&);
    ~Environment();
    auto operator=(const Environment&) -> Environment&;
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    auto operator=(Environment&&) -> Environment&;
    friend void swap(Environment&, Environment&);

    /**
     * Helper function to create a table in the same allocator as the environment.
     */
    [[nodiscard]] auto make_table() const -> Table;

    /**
     * Populates the environment with the (implemented) lua standard library.
     */
    void add_default_stdlib();

    /**
     * Add a variable to the environment.
     */
    void add(const std::string& name, Value value);
    void add(std::string&& name, Value value);

    /**
     * Add a table variable with the given name and return the table.
     */
    auto add_table(const std::string& name) -> Table;

    /**
     * Add multiple variables to the environment.
     */
    void add_all(std::unordered_map<std::string, Value> values);
    void add_all(std::initializer_list<std::pair<std::string, Value>> values);
    void add_all(std::vector<std::pair<std::string, Value>> values);

    /**
     * Get the value of a variable.
     *
     * Throws an exception if the variable does not exist.
     */
    auto get(const std::string& name) -> Value;

    /**
     * Check if a variable is set.
     */
    auto has(const std::string& name) -> bool;

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

    /**
     * Returns the number of variables.
     */
    [[nodiscard]] auto size() const -> size_t;

    // only for internal use
    auto get_raw_impl() -> Impl&;

    friend auto operator==(const Environment&, const Environment&) noexcept -> bool;
    friend auto operator!=(const Environment&, const Environment&) noexcept -> bool;
    friend auto operator<<(std::ostream&, const Environment&) -> std::ostream&;
};

} // namespace minilua

// TODO MOVE TO DIFFERENT FILE

#include "val.hpp"

namespace lua {
namespace rt {

struct Environment : enable_shared_from_this<Environment> {
private:
    table t;
    shared_ptr<Environment> parent;
    table* global = nullptr;

public:
    Environment(const shared_ptr<Environment>& parent) : parent{parent} {
        if (parent) {
            global = parent->global;
        } else {
            global = &t;
        }
    }

    void clear() { t.clear(); }

    void assign(const val& var, const val& newval, bool is_local);
    val getvar(const val& var);

    void populate_stdlib();
};

} // namespace rt
} // namespace lua

#endif
