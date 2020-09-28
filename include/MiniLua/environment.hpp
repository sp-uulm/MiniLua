#ifndef MINILUA_ENVIRONMENT_H
#define MINILUA_ENVIRONMENT_H

#include <iostream>
#include <string>
#include <unordered_map>

#include "utils.hpp"

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
    struct Impl;
    owning_ptr<Impl> impl;

public:
    Environment();
    Environment(const Environment&);
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    Environment(Environment&&);
    ~Environment();
    auto operator=(const Environment&) -> Environment&;
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    auto operator=(Environment &&) -> Environment&;
    friend void swap(Environment&, Environment&);

    /**
     * Similar to the old env->populate_stdlib().
     */
    void add_default_stdlib();

    void add(const std::string& name, Value value);
    void add(std::string&& name, Value value);

    void add_all(std::unordered_map<std::string, Value> values);
    void add_all(std::initializer_list<std::pair<const std::string, Value>> values);

    auto get(const std::string& name) -> Value&;

    void set_stdin(std::istream*);
    void set_stdout(std::ostream*);
    void set_stderr(std::ostream*);

    auto get_stdin() -> std::istream*;
    auto get_stdout() -> std::ostream*;
    auto get_stderr() -> std::ostream*;

    [[nodiscard]] auto size() const -> size_t;

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

    void clear() {
        t.clear();
    }

    void assign(const val& var, const val& newval, bool is_local);
    val getvar(const val& var);

    void populate_stdlib();
};

} // namespace rt
} // namespace lua

#endif
