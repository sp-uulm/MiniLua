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
 * @brief The environment/configuration for the Interpreter.
 *
 * This contains things like global and local variables (including functions),
 * etc. But local variables can not be manually created.
 *
 * The default constructor initializes an empty environment with the standard
 * C++ I/O streams (`std::cin`, `std::cout` and `std::cerr`).
 *
 * Supports equality operators.
 */
class Environment {
public:
    struct Impl;

private:
    owning_ptr<Impl> impl;

public:
    /**
     * @brief Create an empty environment with the default C++ I/O streams
     * in the @ref GLOBAL_ALLOCATOR.
     */
    Environment();
    /**
     * @brief Create an empty environment with the default C++ I/O streams
     * in the given `allocator`.
     */
    Environment(MemoryAllocator* allocator);
    explicit Environment(Impl);
    /**
     * @brief Copy constructor.
     */
    Environment(const Environment&);
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    /**
     * @brief Move constructor.
     */
    Environment(Environment&&);
    ~Environment();
    /**
     * @brief Copy assignment operator.
     */
    auto operator=(const Environment&) -> Environment&;
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    /**
     * @brief Move assignment operator.
     */
    auto operator=(Environment&&) -> Environment&;
    /**
     * @brief Swap function.
     */
    friend void swap(Environment&, Environment&);

    /**
     * @brief Returns the used memory allocator.
     */
    [[nodiscard]] auto allocator() const -> MemoryAllocator*;

    /**
     * @brief Create a new table in the allocator of this environment.
     */
    [[nodiscard]] auto make_table() const -> Table;

    /**
     * @brief Add a variable to the environment.
     */
    void add(const std::string& name, Value value);
    /**
     * @brief Add a variable to the environment.
     */
    void add(std::string&& name, Value value);

    /**
     * @brief Add a table as a variable with the given name and return the
     * table.
     */
    auto add_table(const std::string& name) -> Table;

    /**
     * @brief Add multiple variables to the environment.
     */
    void add_all(std::unordered_map<std::string, Value> values);
    /**
     * @brief Add multiple variables to the environment.
     */
    void add_all(std::initializer_list<std::pair<std::string, Value>> values);
    /**
     * @brief Add multiple variables to the environment.
     */
    void add_all(std::vector<std::pair<std::string, Value>> values);

    /**
     * @brief Get the value of a variable.
     *
     * Returns Nil if the variable does not exist.
     */
    auto get(const std::string& name) -> Value;

    /**
     * @brief Check if a variable is set.
     */
    auto has(const std::string& name) -> bool;

    /**
     * @brief Sets the stdin stream to use in lua code.
     */
    void set_stdin(std::istream*);
    /**
     * @brief Sets the stdout stream to use in lua code.
     */
    void set_stdout(std::ostream*);
    /**
     * @brief Sets the stderr stream to use in lua code.
     */
    void set_stderr(std::ostream*);

    /**
     * @brief Get the configured stdin stream.
     */
    auto get_stdin() -> std::istream*;
    /**
     * @brief Get the configured stdout stream.
     */
    auto get_stdout() -> std::ostream*;
    /**
     * @brief Get the configured stderr stream.
     */
    auto get_stderr() -> std::ostream*;

    /**
     * @brief Returns the number of variables.
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
