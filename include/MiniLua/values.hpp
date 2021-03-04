#ifndef MINILUA_VALUES_HPP
#define MINILUA_VALUES_HPP

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "MiniLua/allocator.hpp"
#include "source_change.hpp"
#include "utils.hpp"

// helper macros to delegate a binary operator
#define DELEGATE_OP(TYPE, OP)                                                                      \
    constexpr auto operator OP(const TYPE& lhs, const TYPE& rhs)->TYPE {                           \
        return TYPE(lhs.value OP rhs.value);                                                       \
    }

namespace minilua {

class Environment;

// helper template used later to repeat a type in fold expression
template <size_t, class T> using T_ = T;

// forward declaration
class Value;

/**
 * @brief A Vallist can contain an arbitrary amount of `Value`s.
 *
 * You can use a Vallist in destructuring assignments. You have to specify the
 * number of values you want. If the number is lower than the amount of values
 * it will simply return the first values. If the number is higher than the
 * amount of values it will return references to a `Nil` value for the remaining
 * values.
 *
 * \brief This will actually return `std::reference_wrapper`s because
 * it's not possible to put references inside a tuple. That means that you have
 * to call `get` on the values to use them:
 *
 * ```cpp
 * auto& [one, two, three] = vallist.tuple<3>();
 * out.get();
 * two.get();
 * ```
 *
 * You can iterate over a Vallist and you can get one element by index using
 * `Vallist::get`.  Iteration only works for the exact number of elements (like
 * for `std::vector`), so you need to be carful not to dereference a pointer
 * into an empty Vallist). `Vallist::get` will return `Nil` values if the
 * element you request is not in the Vallist.
 *
 * Supports equality operators.
 */
class Vallist {
    struct Impl;
    owning_ptr<Impl> impl;

public:
    /**
     * @brief Creates an empty Vallist.
     */
    Vallist();
    /**
     * @brief Creates a Vallist with one Value.
     */
    explicit Vallist(Value);
    /**
     * @brief Creates a Vallist with a vector of Value.
     */
    Vallist(std::vector<Value>);
    /**
     * @brief Creates a Vallist with a multiple `Value`s.
     */
    Vallist(std::initializer_list<Value>);
    /**
     * @brief Concatenate multiple `Vallist`s.
     */
    Vallist(std::vector<Vallist>);

    /**
     * @brief Copy constructor.
     */
    Vallist(const Vallist&);
    /**
     * @brief Move constructor.
     */
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    Vallist(Vallist&&);
    /**
     * @brief Copy assignment operator.
     */
    auto operator=(const Vallist&) -> Vallist&;
    /**
     * @brief Move assignment operator.
     */
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    auto operator=(Vallist&&) -> Vallist&;
    ~Vallist();

    /**
     * @brief The number of actual `Value`s in the Vallist.
     *
     * \note This might be different than the number of non-`Nil` values.
     */
    [[nodiscard]] auto size() const -> size_t;

    /**
     * @brief The value at the given index.
     *
     * If the value does not exist a reference to a Nil value will be returned.
     */
    [[nodiscard]] auto get(size_t index) const -> const Value&;

    /**
     * @brief Iterator over the Vallist. Can be used in e.g. for loops.
     */
    [[nodiscard]] auto begin() const -> std::vector<Value>::const_iterator;
    /**
     * @brief Iterator over the Vallist. Can be used in e.g. for loops.
     */
    [[nodiscard]] auto end() const -> std::vector<Value>::const_iterator;

    // helper for next method
    template <std::size_t... Is>
    [[nodiscard]] auto tuple(std::index_sequence<Is...> /*unused*/) const
        -> std::tuple<T_<Is, std::reference_wrapper<const Value>>...> {
        return std::make_tuple(std::cref(this->get(Is))...);
    }

    /**
     * @brief Returns a number of values in a tuple.
     *
     * The number of returned values depends on the template parameter.
     *
     * If the given number is bigger than the number of values the remaining
     * items in the tuple will be filled with Nil.
     *
     * Use this for structured binding declarations:
     *
     * ```cpp
     * const auto& [val1, val2, val3] = vallist.tuple<3>();
     * ```
     *
     * \note The values will be `std::reference_wrapper`s because it's not
     * possible to put references in a tuple. You have to call `get` on the
     * values before using them.
     */
    template <std::size_t N> [[nodiscard]] auto tuple() const {
        return tuple(std::make_index_sequence<N>{});
    }

    /**
     * @brief Equality comparison.
     */
    friend auto operator==(const Vallist&, const Vallist&) -> bool;
    /**
     * @brief Unequality comparison.
     */
    friend auto operator<<(std::ostream&, const Vallist&) -> std::ostream&;
};

/**
 * @brief A lua `nil` value.
 *
 * Supports equality operators.
 *
 * Is hashable.
 */
struct Nil {
    /**
     * @brief The type of this value as a string.
     */
    constexpr static const std::string_view TYPE = "nil";

    /**
     * @brief Converts the value to it's literal representation.
     */
    [[nodiscard]] auto to_literal() const -> std::string;

    /**
     * @brief Convert the value to a bool (always `false`).
     */
    explicit operator bool() const;
};
constexpr auto operator==(Nil /*unused*/, Nil /*unused*/) noexcept -> bool { return true; }
constexpr auto operator!=(Nil /*unused*/, Nil /*unused*/) noexcept -> bool { return false; }
auto operator<<(std::ostream&, Nil) -> std::ostream&;

/**
 * @brief A lua boolean value.
 *
 * Supports equality operators and `&&`, `||` and `^`.
 *
 * Is hashable.
 */
struct Bool {
    /**
     * @brief The bool value.
     */
    bool value; // NOLINT(misc-non-private-member-variables-in-classes)

    /**
     * @brief The type of this value as a string.
     */
    constexpr static const std::string_view TYPE = "boolean";

    /**
     * @brief Create a Bool from a `bool`.
     */
    constexpr Bool(bool value) : value(value) {}

    /**
     * @brief Converts the value to it's literal representation.
     */
    [[nodiscard]] auto to_literal() const -> std::string;

    /**
     * @brief Convert the value to a bool (uses the underlying bool).
     */
    explicit operator bool() const;
};
constexpr auto operator==(Bool lhs, Bool rhs) noexcept -> bool { return lhs.value == rhs.value; }
constexpr auto operator!=(Bool lhs, Bool rhs) noexcept -> bool { return !(lhs == rhs); }
auto operator<<(std::ostream&, Bool) -> std::ostream&;

// normal c++ operators
DELEGATE_OP(Bool, &&);
DELEGATE_OP(Bool, ||);
DELEGATE_OP(Bool, ^);

/**
 * Numbers in lua can be integers or floats.
 *
 * \note Int and Float does not have the same meaning as in C++.
 * See @ref Number::Int and @ref Number::Float.
 *
 * The Number type automatically converts Int to Float according to the rules
 * of lua.
 *
 * The equals and hash implementation treat whole Floats and like their
 * equivalent Ints.
 *
 * Supports comparison operators and `+`, `-`, `*`, `/`, `^` (exponentiation),
 * `%`, `&`, `|`.
 *
 * Is hashable.
 */
class Number {
public:
    using Int = long;
    using Float = double;

private:
    std::variant<Int, Float> value;

public:
    /**
     * @brief The type of this value as a string.
     */
    constexpr static const std::string_view TYPE = "number";

    /**
     * @brief Creates a number form an int.
     */
    constexpr Number(int value) : value((Int)value) {}
    /**
     * @brief Creates a number form a Int.
     */
    constexpr Number(Int value) : value(value) {}
    /**
     * @brief Creates a number form a Float.
     */
    constexpr Number(Float value) : value(value) {}

    /**
     * @brief Converts the value to it's literal representation.
     */
    [[nodiscard]] auto to_literal() const -> std::string;

    /**
     * @brief Convert the value to a bool (always `true`).
     */
    explicit operator bool() const;

    /**
     * @brief Converts the number to a float.
     *
     * If it is an int otherwise just returns the number.
     *
     * \note This actually returns a double but they are called float in lua.
     */
    [[nodiscard]] auto as_float() const -> Float;
    /**
     * @brief Returns the number as an int if it is a whole number.
     *
     * Otherwise throws an exception.
     *
     * @throws std::runtime_error
     */
    [[nodiscard]] auto try_as_int() const -> Int;

    /**
     * @brief Convert to Int.
     *
     * By casting the Float to an Int (if it is not already an Int).
     */
    [[nodiscard]] auto convert_to_int() const -> Int;

    [[nodiscard]] auto raw() const -> std::variant<Int, Float>;

    /**
     * @brief Check if the number is an int.
     */
    [[nodiscard]] auto is_int() const -> bool;
    [[nodiscard]] auto is_float() const -> bool;

    template <typename Fn> auto visit(Fn fn) const {
        static_assert(std::is_invocable_v<Fn, Int>, "fn must be invocable with an int parameter");
        static_assert(
            std::is_invocable_v<Fn, Float>, "fn must be invocable with a double parameter");

        return std::visit(fn, this->value);
    }

    /**
     * @brief Invoke the given function with either two Ints or two Floats.
     *
     * If one of the numbers (either this or rhs) is a Float the other one will
     * also be converted to a Float if it is not one.
     */
    template <typename Fn> auto apply_with_number_rules(const Number& rhs, Fn fn) const {
        static_assert(std::is_invocable_v<Fn, Int, Int>);
        static_assert(std::is_invocable_v<Fn, Float, Float>);

        return std::visit(
            overloaded{
                [&fn](Int lhs, Int rhs) { return fn(lhs, rhs); },
                [&fn](Float lhs, Int rhs) { return fn(lhs, static_cast<double>(rhs)); },
                [&fn](Int lhs, Float rhs) { return fn(static_cast<double>(lhs), rhs); },
                [&fn](Float lhs, Float rhs) { return fn(lhs, rhs); },
            },
            this->value, rhs.value);
    }

    friend auto operator==(Number lhs, Number rhs) noexcept -> bool;
    friend auto operator!=(Number lhs, Number rhs) noexcept -> bool;
    friend auto operator<(Number lhs, Number rhs) noexcept -> bool;
    friend auto operator>(Number lhs, Number rhs) noexcept -> bool;
    friend auto operator<=(Number lhs, Number rhs) noexcept -> bool;
    friend auto operator>=(Number lhs, Number rhs) noexcept -> bool;

    friend auto operator<<(std::ostream&, Number) -> std::ostream&;

    friend auto operator-(Number self) -> Number;
    friend auto operator+(const Number& lhs, const Number& rhs) -> Number;
    friend auto operator-(const Number& lhs, const Number& rhs) -> Number;
    friend auto operator*(const Number& lhs, const Number& rhs) -> Number;
    friend auto operator/(const Number& lhs, const Number& rhs) -> Number;

    [[nodiscard]] auto int_div(const Number& rhs) const -> Number;
    // We don't use the "normal" C++ operators because they don't cover all lua operators
    [[nodiscard]] auto pow(const Number& rhs) const -> Number;
    [[nodiscard]] auto mod(const Number& rhs) const -> Number;
    // We can't give these methods propert names because C++ has alternate operator tokens
    // In particular using bitand, bitor, and, or and not is illegal syntax
    [[nodiscard]] auto bit_and(const Number& rhs) const -> Number;
    [[nodiscard]] auto bit_or(const Number& rhs) const -> Number;
    [[nodiscard]] auto bit_xor(const Number& rhs) const -> Number;
    [[nodiscard]] auto bit_shl(const Number& rhs) const -> Number;
    [[nodiscard]] auto bit_shr(const Number& rhs) const -> Number;
    [[nodiscard]] auto bit_not() const -> Number;
    [[nodiscard]] auto logic_and(const Number& rhs) const -> Number;
    [[nodiscard]] auto logic_or(const Number& rhs) const -> Number;
};

/**
 * @brief A lua string value.
 *
 * Supports comparison operators.
 *
 * Is hashable.
 */
struct String {
    std::string value; // NOLINT(misc-non-private-member-variables-in-classes)

    /**
     * @brief The type of this value as a string.
     */
    constexpr static const std::string_view TYPE = "string";

    /**
     * @brief Create a String from a `std::string`.
     */
    String(std::string value);

    /**
     * @brief Converts the value to it's literal representation.
     */
    [[nodiscard]] auto to_literal() const -> std::string;

    /**
     * @brief Check if the String is a valid lua identifier.
     */
    [[nodiscard]] auto is_valid_identifier() const -> bool;

    /**
     * @brief Convert the value to a bool (always `true`).
     */
    explicit operator bool() const;

    /**
     * @brief Swap function.
     */
    friend void swap(String& self, String& other);
};
auto operator==(const String& a, const String& b) noexcept -> bool;
auto operator!=(const String& a, const String& b) noexcept -> bool;
inline auto operator<(const String& lhs, const String& rhs) noexcept -> bool {
    return lhs.value < rhs.value;
}
inline auto operator>(const String& lhs, const String& rhs) noexcept -> bool {
    return lhs.value > rhs.value;
}
inline auto operator<=(const String& lhs, const String& rhs) noexcept -> bool {
    return !(lhs > rhs);
}
inline auto operator>=(const String& lhs, const String& rhs) noexcept -> bool {
    return !(lhs < rhs);
}

auto operator<<(std::ostream&, const String&) -> std::ostream&;

// Forward declaration
class CallContext;

struct TableImpl;

/**
 * @brief A lua table value.
 *
 * Table is basically a `std::map`. Aditionally table is aliasable (i.e. it
 * acts like a `std::sharded_ptr`). That means two variables (or two `Table`
 * `Value`s) can refer to the same actual table.
 *
 * \warning You need to be careful to only nest tables that were created using the
 * same `MemoryAllocator`.
 *
 * \warning It would be ok to nest table created using the @ref GLOBAL_ALLOCATOR
 * inside table created using other allocators but the not other way around.
 *
 * Support equality operators.
 *
 * Is hashable.
 */
class Table {
private:
    // The impl pointer is allocated through this allocator and will also
    // be freed through it.
    MemoryAllocator* allocator;
    TableImpl* impl;

public:
    // iterator definitions
    //
    // NOTE: it's not possible to use the underlying std::unordered_map<Value, Value>::iterator
    // because Value is not a complete type at this location. This means we can't
    // use it directly for begin() and end() and we can't use it in the definition
    // of iterator/const_iterator.
    using allocator_type = std::allocator<std::pair<const Value, Value>>;
    using value_type = allocator_type::value_type;
    using reference = allocator_type::reference;
    using const_reference = allocator_type::const_reference;
    using size_type = allocator_type::size_type;

    /**
     * @brief Iterator through a Table.
     */
    class iterator {
        friend class Table;

        struct Impl;
        owning_ptr<Impl> impl;

    public:
        using difference_type = allocator_type::difference_type;
        using value_type = allocator_type::value_type;
        using reference = allocator_type::reference;
        using pointer = allocator_type::pointer;
        using iterator_category = std::forward_iterator_tag;

        iterator();
        iterator(const iterator&);
        ~iterator();

        auto operator=(const iterator&) -> iterator&;
        auto operator==(const iterator&) const -> bool;
        auto operator!=(const iterator&) const -> bool;

        auto operator++() -> iterator&;
        auto operator++(int) -> iterator;

        auto operator*() const -> reference;
        auto operator->() const -> pointer;
    };

    /**
     * @brief Const iterator through a Table.
     */
    class const_iterator {
        friend class Table;

        struct Impl;
        owning_ptr<Impl> impl;

    public:
        using difference_type = allocator_type::difference_type;
        using value_type = allocator_type::value_type;
        using reference = allocator_type::const_reference;
        using pointer = allocator_type::const_pointer;
        using iterator_category = std::forward_iterator_tag;

        const_iterator();
        const_iterator(const const_iterator&);
        ~const_iterator();

        auto operator=(const const_iterator&) -> const_iterator&;
        auto operator==(const const_iterator&) const -> bool;
        auto operator!=(const const_iterator&) const -> bool;

        auto operator++() -> const_iterator&;
        auto operator++(int) -> const_iterator;

        auto operator*() const -> reference;
        auto operator->() const -> pointer;
    };

    /**
     * @brief The type of this value as a string.
     */
    constexpr static const std::string_view TYPE = "table";

    // NOTE: default constructor has to be separate from the one only taking the
    // allocator and we can't use default arguments there.
    /**
     * @brief Creates an empty table in the @ref GLOBAL_ALLOCATOR.
     */
    Table();
    /**
     * @brief Creates an empty table in the given allocator.
     */
    Table(MemoryAllocator* allocator);
    /**
     * @brief Creates and fills a table in the given allocator.
     */
    Table(std::unordered_map<Value, Value>, MemoryAllocator* allocator = &GLOBAL_ALLOCATOR);
    /**
     * @brief Creates and fills a table in the given allocator.
     */
    Table(
        std::initializer_list<std::pair<const Value, Value>> values,
        MemoryAllocator* allocator = &GLOBAL_ALLOCATOR);

    /**
     * @brief Copy a table to a different allocator.
     *
     * This will make a deep copy meaning all nested tables will also be copied
     * to the allocator.
     *
     * \warning Currently this does not support cyclic table nesting.
     */
    Table(const Table& other, MemoryAllocator* allocator);

    /**
     * @brief Copy constructor.
     */
    Table(const Table& other);
    /**
     * @brief Move constructor.
     */
    Table(Table&& other) noexcept;
    ~Table() noexcept;
    /**
     * @brief Copy assignment operator.
     */
    auto operator=(const Table& other) -> Table&;
    /**
     * @brief Move assignment operator.
     */
    auto operator=(Table&& other) noexcept -> Table&;
    /**
     * @brief Swap function.
     */
    friend void swap(Table& self, Table& other);

    /**
     * @brief The result of the lua length operator `#`.
     *
     * Satisfies: `(border == 0 or t[border] ~= nil) and t[border + 1] == nil`
     */
    [[nodiscard]] auto border() const -> int;

    /**
     * @brief Try to get the value with the given key.
     *
     * If the value does not exist this will return `Nil`.
     */
    auto get(const Value& key) -> Value;
    /**
     * @brief Check if the table has a value for the given key.
     *
     * \note The table might still contain a `Nil` value for the key.
     */
    auto has(const Value& key) -> bool;
    /**
     * @brief Sets the key to value.
     */
    void set(const Value& key, Value value);
    /**
     * @brief Sets the key to value.
     */
    void set(Value&& key, Value value);
    /**
     * Copy the `other` table into this table overwriting all keys that are
     * duplicate.
     */
    void set_all(const Table& other);

    /**
     * @brief The number of values in the table.
     */
    [[nodiscard]] auto size() const -> size_t;

    /**
     * @brief Returns an iterator to the beginning.
     */
    auto begin() -> iterator;
    /**
     * @brief Returns an iterator to the beginning.
     */
    [[nodiscard]] auto begin() const -> const_iterator;
    /**
     * @brief Returns an iterator to the beginning.
     */
    [[nodiscard]] auto cbegin() const -> const_iterator;
    /**
     * @brief Returns an iterator to the end.
     */
    auto end() -> iterator;
    /**
     * @brief Returns an iterator to the end.
     */
    [[nodiscard]] auto end() const -> const_iterator;
    /**
     * @brief Returns an iterator to the end.
     */
    [[nodiscard]] auto cend() const -> const_iterator;

    /**
     * @brief Converts the value to it's literal representation.
     */
    [[nodiscard]] auto to_literal() const -> std::string;

    /**
     * @brief The next index of the table and its associated value after index.
     *
     * If there is no value at the index an exception is thrown.
     *
     * @param key The index you want to get the next element.
     *
     * @return An empty `Vallist` when called with the last index or on an empty
     * table. Else it returns the next index and its associated value.
     */
    [[nodiscard]] auto next(const Value& key) const -> Vallist;

    /**
     * @brief Equality comparions.
     *
     * Does not compare the content of two tables, only if the table actually
     * represent the same table.
     */
    friend auto operator==(const Table&, const Table&) noexcept -> bool;
    /**
     * @brief Unequality comparions.
     *
     * Does not compare the content of two tables, only if the table actually
     * represent the same table.
     */
    friend auto operator!=(const Table&, const Table&) noexcept -> bool;
    friend auto operator<<(std::ostream&, const Table&) -> std::ostream&;

    friend struct std::hash<Table>;

    // TODO maybe return proxy "entry" type to avoid unnecessary Nil values
    /**
     * @brief Access a value by key.
     */
    auto operator[](const Value&) -> Value&;
    /**
     * @brief Access a value by key.
     */
    auto operator[](const Value&) const -> const Value&;

    /**
     * @brief Convert the value to a bool (always `true`).
     */
    explicit operator bool() const;
};

struct BinaryOrigin;
struct UnaryOrigin;

/**
 * @brief Argument for lua `Function`s.
 *
 * Contains information for use in the implementation of native (and lua)
 * functions.
 *
 * Contains the arguments and the environment.
 *
 * \warning
 * The `CallContext` and the `Environment` should not be copied and stored
 * somewhere that outlives the function call. Because the environment may be
 * freed after the call to the function ends. You can however safely store any
 * `Value`s or a copy of the arguments.
 */
class CallContext {
    struct Impl;
    owning_ptr<Impl> impl;

public:
    /**
     * @brief Create a CallContext from the Environment.
     *
     * \warning Only for internal use. Be careful that the Environment pointer
     * outlives the CallContext.
     */
    CallContext(Environment* env);
    /**
     * @brief Copy constructor.
     */
    CallContext(const CallContext&);
    /**
     * @brief Move constructor.
     */
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    CallContext(CallContext&&);
    /**
     * @brief Copy assignment operator.
     */
    auto operator=(const CallContext&) -> CallContext&;
    /**
     * @brief Move assignment operator.
     */
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    auto operator=(CallContext&&) -> CallContext&;
    ~CallContext();

    /**
     * @brief Create a new call context with new arguments.
     *
     * It reuses all other information.
     */
    [[nodiscard]] auto make_new(Vallist, std::optional<Range> location = std::nullopt) const
        -> CallContext;

    /**
     * @brief Create a new call context with new arguments.
     *
     * It reuses all other information.
     */
    template <typename... Args>
    [[nodiscard]] auto make_new(Args... args, std::optional<Range> location = std::nullopt) const
        -> CallContext {
        return this->make_new(Vallist{args...}, location);
    }

    /**
     * @brief Create a new table with the same allocator as the environment.
     */
    [[nodiscard]] auto make_table() const -> Table;

    /**
     * @brief The location of the call.
     *
     * Can be `std::nullopt` if the CallContext was not created by a lua
     * function call.
     */
    [[nodiscard]] auto call_location() const -> std::optional<Range>;

    /**
     * @brief Returns a reference to the environment.
     *
     * For `Function`s created in C++ you can only access the global
     * environment.
     *
     * For functions created in lua you can access the global environment and
     * local variables that were in scope when creating the function.
     */
    [[nodiscard]] auto environment() const -> Environment&;

    /**
     * @brief Returns the value of a variable accessible from the function.
     *
     * Returns `Nil` if the variable is not accessible or does not exist.
     */
    [[nodiscard]] auto get(const std::string& name) const -> Value;

    /**
     * @brief Returns the arguments given to this function.
     */
    [[nodiscard]] auto arguments() const -> const Vallist&;

    /**
     * @brief Convenience method for writing unary numeric functions.
     *
     * Convenience method for writing functions with one numeric argument
     * that should track the origin (e.g. pow).
     *
     * Usage:
     *
     * ```cpp
     * auto [arg, origin] = ctx.unary_numeric_arg_helper();
     * origin.reverse = unary_num_reverse(...);
     * // ...
     * ```
     *
     * See also: `CallContext::binary_numeric_args_helper`.
     */
    [[nodiscard]] auto unary_numeric_arg_helper() const -> std::tuple<Number, UnaryOrigin>;
    /**
     * @brief Convenience method for writing unary numeric functions.
     *
     * Convenience method for writing functions with two numeric arguments
     * that should track the origin (e.g. sqrt).
     *
     * Usage:
     *
     * ```cpp
     * auto [arg1, arg2, origin] = ctx.binary_numeric_arg_helper();
     * origin.reverse = binary_num_reverse(...);
     * // ...
     * ```
     *
     * See also: `CallContext::unary_numeric_arg_helper`.
     */
    [[nodiscard]] auto binary_numeric_args_helper() const
        -> std::tuple<Number, Number, BinaryOrigin>;

    friend auto operator<<(std::ostream&, const CallContext&) -> std::ostream&;
};

/**
 * @brief Result of calling a lua Function.
 *
 * Contains the actual return value (actually a Vallist) and optionally source
 * changes.
 *
 * Supports equality operators.
 */
class CallResult {
    Vallist vallist;
    std::optional<SourceChangeTree> _source_change;

public:
    /**
     * @brief Creates a CallResult with a Nil value and no source changes.
     */
    CallResult();
    /**
     * @brief Creates a CallResult from a Vallist and no source changes.
     */
    CallResult(Vallist);
    /**
     * @brief Creates a CallResult from multiple values and no source changes.
     */
    CallResult(std::vector<Value>);
    /**
     * @brief Creates a CallResult from multiple values and no source changes.
     */
    explicit CallResult(std::initializer_list<Value>);
    /**
     * @brief Creates a CallResult with a Nil value and the given source
     * changes.
     */
    explicit CallResult(SourceChangeTree);
    /**
     * @brief Creates a CallResult with a Nil value and the given optional
     * source changes.
     */
    explicit CallResult(std::optional<SourceChangeTree>);
    /**
     * @brief Creates a CallResult with the given values and the given source
     * changes.
     */
    explicit CallResult(Vallist, SourceChangeTree);
    /**
     * @brief Creates a CallResult with the given values and the given optional
     * source changes.
     */
    explicit CallResult(Vallist, std::optional<SourceChangeTree>);

    /**
     * @brief Get the return values.
     */
    [[nodiscard]] auto values() const -> const Vallist&;
    /**
     * @brief Get the source change.
     */
    [[nodiscard]] auto source_change() const -> const std::optional<SourceChangeTree>&;
};

auto operator==(const CallResult&, const CallResult&) -> bool;
auto operator!=(const CallResult&, const CallResult&) -> bool;
auto operator<<(std::ostream&, const CallResult&) -> std::ostream&;

/**
 * @brief A lua function value.
 *
 * A function (in lua or implemented natively in C++).
 *
 * Notes for implementing native functions:
 *
 * You get a `CallContext` as only argument. You can retrieve the actual
 * arguments and the global environment from the context.
 *
 * When using lua `Value`s you can use the normal C++ operators. Note that `^`
 * performs the pow operation like in lua (and not xor like usually in C++).
 * These operators will track the origin and can be later forced to a different
 * value.
 *
 * Care must be taken when there are control flow constructs in your native
 * function (e.g. `if`, `while`, ...). Because this might break the source
 * change mechanism if you use operators on `Value`s. If you use control flow
 * constructs you should remove the origin from the returned Value with
 * `Value::remove_origin`.
 *
 * Is hashable.
 */
class Function {
    using FnType = CallResult(CallContext);

    std::shared_ptr<std::function<FnType>> func;
    std::string name;

public:
    /**
     * @brief The type of this value as a string.
     */
    constexpr static const std::string_view TYPE = "function";

    /**
     * @brief Create a function from a callable (e.g. lambda, function
     * pointer).
     */
    template <typename Fn, typename = std::enable_if_t<std::is_invocable_v<Fn, CallContext>>>
    Function(Fn fn) : Function(fn, "") {}

    /**
     * @brief Create a function from a callable and the given name.
     */
    template <typename Fn, typename = std::enable_if_t<std::is_invocable_v<Fn, CallContext>>>
    Function(Fn fn, std::string name)
        : func(std::make_shared<std::function<FnType>>()), name(std::move(name)) {
        if constexpr (std::is_convertible_v<Fn, std::function<FnType>>) {
            *this->func = fn;
        } else if constexpr (std::is_convertible_v<std::invoke_result_t<Fn, CallContext>, Value>) {
            // easy use of functions that return a type that is convertible to Value (e.g. string)
            *this->func = [fn](CallContext ctx) -> CallResult { return CallResult({fn(ctx)}); };
        } else if constexpr (std::is_void_v<std::invoke_result_t<Fn, CallContext>>) {
            // support void functions by returning an empty Vallist
            *this->func = [fn](CallContext ctx) -> CallResult {
                fn(ctx);
                return CallResult();
            };
        } else {
            // can't use false, because value has to depend on type parameter otherwise this will
            // not compile
            static_assert(
                std::is_void<Fn>(),
                "can only use function likes that take a CallContext as parameter");
        }
    }

    /**
     * @brief Converts the value to it's literal representation.
     *
     * Always throws an exception. Just here for convenience.
     */
    [[nodiscard]] auto to_literal() const -> std::string;

    /**
     * @brief Calls the function.
     */
    [[nodiscard]] auto call(CallContext) const -> CallResult;

    /**
     * @brief Convert the value to a bool (always `true`).
     */
    explicit operator bool() const;

    /**
     * @brief Swap function.
     */
    friend void swap(Function& self, Function& other);

    friend struct std::hash<Function>;
};

auto operator<<(std::ostream&, const Function&) -> std::ostream&;

} // namespace minilua

namespace std {

// NOTE: has to come before definition of Value
template <> struct hash<minilua::Value> {
    auto operator()(const minilua::Value& value) const -> size_t;
};
template <> struct hash<minilua::Nil> {
    auto operator()(const minilua::Nil& value) const -> size_t;
};
template <> struct hash<minilua::Bool> {
    auto operator()(const minilua::Bool& value) const -> size_t;
};
template <> struct hash<minilua::Number> {
    auto operator()(const minilua::Number& value) const -> size_t;
};
template <> struct hash<minilua::String> {
    auto operator()(const minilua::String& value) const -> size_t;
};
template <> struct hash<minilua::Table> {
    auto operator()(const minilua::Table& value) const -> size_t;
};
template <> struct hash<minilua::Function> {
    auto operator()(const minilua::Function& value) const -> size_t;
};

} // namespace std

namespace minilua {

/**
 * @brief Default origin for `Value`s.
 *
 * Supports equality operators.
 */
struct NoOrigin {};
auto operator==(const NoOrigin&, const NoOrigin&) noexcept -> bool;
auto operator!=(const NoOrigin&, const NoOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const NoOrigin&) -> std::ostream&;

/**
 * @brief Can be used for externally produced values but is mainly a
 * placeholder.
 *
 * \todo future use could include allowing to specify a function to change the
 * external value (like it is possible for values produced from literals).
 *
 * Support equality operators.
 */
struct ExternalOrigin {};
auto operator==(const ExternalOrigin&, const ExternalOrigin&) noexcept -> bool;
auto operator!=(const ExternalOrigin&, const ExternalOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const ExternalOrigin&) -> std::ostream&;

/**
 * @brief Origin for a Value that was created from a literal in code.
 *
 * Supports equality operators.
 */
struct LiteralOrigin {
    /**
     * @brief The range of the literal.
     */
    Range location;
};

auto operator==(const LiteralOrigin&, const LiteralOrigin&) noexcept -> bool;
auto operator!=(const LiteralOrigin&, const LiteralOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const LiteralOrigin&) -> std::ostream&;

/**
 * @brief Origin for a Value that was created in a binary operation (or some
 * functions with two arguments).
 *
 * Support equality operators.
 */
struct BinaryOrigin {
    using ReverseFn = std::optional<SourceChangeTree>(const Value&, const Value&, const Value&);

    /**
     * @brief The first value used to call the binary operator or function.
     */
    owning_ptr<Value> lhs;
    /**
     * @brief The second value used to call the binary operator or function.
     */
    owning_ptr<Value> rhs;
    /**
     * @brief The range of the operator or function call.
     */
    std::optional<Range> location;
    /**
     * @brief The reverse function.
     *
     * Takes as parameters: `new_value`, `old_lhs` and `old_rhs` and returns
     * `std::optional<SourceChangeTree>`.
     */
    std::function<ReverseFn> reverse;
};

auto operator==(const BinaryOrigin&, const BinaryOrigin&) noexcept -> bool;
auto operator!=(const BinaryOrigin&, const BinaryOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const BinaryOrigin&) -> std::ostream&;

/**
 * @brief Origin for a Value that was created in a unary operation
 * (or some functions with one argument).
 *
 * Support equality operators.
 */
struct UnaryOrigin {
    using ReverseFn = std::optional<SourceChangeTree>(const Value&, const Value&);

    /**
     * @brief The value used to call the unary operator or function.
     */
    owning_ptr<Value> val;
    /**
     * @brief The range of the operator or function call.
     */
    std::optional<Range> location;
    /**
     * @brief The reverse function.
     *
     * Takes as parameters: `new_value` and `old_value` and returns
     * `std::optional<SourceChangeTree>`.
     */
    std::function<ReverseFn> reverse;
};

auto operator==(const UnaryOrigin&, const UnaryOrigin&) noexcept -> bool;
auto operator!=(const UnaryOrigin&, const UnaryOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const UnaryOrigin&) -> std::ostream&;

/**
 * Origin for a Value that was created in a n-ary operation (a function that receives n arguments)
 * (or some functions with one argument) using val.
 */
struct MultipleArgsOrigin {
    using ReverseFn = std::optional<SourceChangeTree>(const Value&, const Vallist&);

    Vallist values;
    std::optional<Range> location;
    // new_value, old_values
    std::function<ReverseFn> reverse;
};

auto operator==(const MultipleArgsOrigin&, const MultipleArgsOrigin&) noexcept -> bool;
auto operator!=(const MultipleArgsOrigin&, const MultipleArgsOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const MultipleArgsOrigin&) -> std::ostream&;

/**
 * @brief The origin of a value.
 *
 * Defaults to `NoOrigin`.
 *
 * Using `BinaryOrigin` and `UnaryOrigin` you can build a tree to track the
 * changes made to a Value while running a lua program.
 *
 * You can manually walk the tree using the variant returned by `Origin::raw`.
 *
 * Supports equality operators.
 */
class Origin {
public:
    using Type = std::variant<
        NoOrigin, ExternalOrigin, LiteralOrigin, BinaryOrigin, UnaryOrigin, MultipleArgsOrigin>;

private:
    Type origin;

public:
    /**
     * @brief Creates an Origin using NoOrigin.
     */
    Origin();
    /**
     * @brief Creates an Origin from the underlying variant type.
     */
    explicit Origin(Type);
    /**
     * @brief Creates an Origin from NoOrigin.
     */
    Origin(NoOrigin);
    /**
     * @brief Creates an Origin from an ExternalOrigin.
     */
    Origin(ExternalOrigin);
    /**
     * @brief Creates an Origin from a LiteralOrigin.
     */
    Origin(LiteralOrigin);
    /**
     * @brief Creates an Origin from a BinaryOrigin.
     */
    Origin(BinaryOrigin);
    /**
     * @brief Creates an Origin from an UnaryOrigin.
     */
    Origin(UnaryOrigin);
    Origin(MultipleArgsOrigin);

    /**
     * @brief Returns the underlying variant type.
     */
    [[nodiscard]] auto raw() const -> const Type&;
    /**
     * @brief Returns the underlying variant type.
     */
    auto raw() -> Type&;

    /**
     * @brief Check if the origin is NoOrigin.
     */
    [[nodiscard]] auto is_none() const -> bool;
    /**
     * @brief Check if the origin is an ExternalOrigin.
     */
    [[nodiscard]] auto is_external() const -> bool;
    /**
     * @brief Check if the origin is a LiteralOrigin.
     */
    [[nodiscard]] auto is_literal() const -> bool;
    /**
     * @brief Check if the origin is a BinaryOrigin.
     */
    [[nodiscard]] auto is_binary() const -> bool;
    /**
     * @brief Check if the origin is a UnaryOrigin.
     */
    [[nodiscard]] auto is_unary() const -> bool;

    /**
     * @brief Uses the reverse function to try to force the result value to
     * change.
     */
    [[nodiscard]] auto force(const Value&) const -> std::optional<SourceChangeTree>;

    /**
     * Sets the file of the underlying origin type (if possible).
     */
    void set_file(std::optional<std::shared_ptr<std::string>> file);
};

auto operator==(const Origin&, const Origin&) noexcept -> bool;
auto operator!=(const Origin&, const Origin&) noexcept -> bool;
auto operator<<(std::ostream&, const Origin&) -> std::ostream&;

} // namespace minilua

/**
 * @brief Contains some overloaded functions and specializations from the `std`
 * namespace.
 */
namespace std {

/**
 * @brief Behaves like `std::get(std::variant)` but only accepts types as
 * template parameter.
 */
template <typename T> auto get(minilua::Origin& origin) -> T& { return std::get<T>(origin.raw()); }
/**
 * @brief Behaves like `std::get(std::variant)` but only accepts types as
 * template parameter.
 */
template <typename T> auto get(const minilua::Origin& origin) -> const T& {
    return std::get<T>(origin.raw());
}

} // namespace std

namespace minilua {

/**
 * @brief Represents a value in lua.
 *
 * You can use most normal C++ operators on these value (`+`, `-`, `*`, `/`,
 * `[]`, ...). If the operation can't be performed on the actual value type an
 * exception will be thrown. Logical operators are implemented like they work
 * in lua. I.e.
 *
 * ```lua
 * "hi" && true == "hi"
 * ```
 *
 * There are also variants of all the operators (as methods) that track the
 * origin of the values.
 *
 * You can get the underlying value either via `std::visit(lambdas,
 * value.raw())` or using `std::get<T>(value)` where `T` is any of the
 * underlying types.
 *
 * Most values can not be changed (i.e. if you add two numbers you create a new
 * value). But you can manually change the underlying variant type.
 *
 * `Function` and `Table` are different in that they act like reference (or act
 * like they are behind a `std::shared_ptr`). I.e. two variables can refer to
 * the same table and function.  In the case of function this is irrelevant
 * because (from the outside) they are immutable.  However tables can be
 * mutated from multiple variables.
 */
class Value {
    struct Impl;
    owning_ptr<Impl> impl;

public:
    using Type = std::variant<Nil, Bool, Number, String, Table, Function>;

    /**
     * @brief Creates a Nil value.
     */
    Value();
    /**
     * @brief Creates a value using the underlying variant type.
     */
    Value(Type val);
    /**
     * @brief Creates a value using the given Nil value.
     */
    Value(Nil val);
    /**
     * @brief Creates a value using the given Bool value.
     */
    Value(Bool val);
    /**
     * @brief Creates a value using the given bool value.
     */
    Value(bool val);
    /**
     * @brief Creates a value using the given Number value.
     */
    Value(Number val);
    /**
     * @brief Creates a value using the given int value.
     */
    Value(int val);
    Value(long val);
    /**
     * @brief Creates a value using the given double value.
     */
    Value(double val);
    /**
     * @brief Creates a value using the given String value.
     */
    Value(String val);
    /**
     * @brief Creates a value using the given `std::string` value.
     */
    Value(std::string val);
    /**
     * @brief Creates a value using the given string pointer value.
     */
    Value(const char* val);
    /**
     * @brief Creates a value using the given Table value.
     */
    Value(Table val);
    /**
     * @brief Creates a value using the given Function value.
     */
    Value(Function val);

    /**
     * @brief Overloaded constructor usable with references to lambdas and
     * function pointers.
     *
     * See `Function`.
     *
     * \note Functions with a parameter of `CallContext&` do not work.
     * Only `CallContext` and `const CallContext&` works.
     */
    template <typename Fn, typename = std::enable_if_t<std::is_invocable_v<Fn, CallContext>>>
    Value(Fn val) : Value(Function(std::forward<Fn>(val))) {}

    /**
     * @brief Copies the value to another allocator.
     *
     * This only has an effect for `Table`s. Other values are simply copied.
     * Table will be deep copied to the given allocators. That means all nested
     * tables will also be copied.
     */
    Value(const Value&, MemoryAllocator* allocator);

    /**
     * @brief Copy constructor.
     */
    Value(const Value&);
    /**
     * @brief Move constructor.
     */
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    Value(Value&&);
    /**
     * @brief Copy assignment operator.
     */
    auto operator=(const Value& other) -> Value&;
    /**
     * @brief Move assignment operator.
     */
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    auto operator=(Value&& other) -> Value&;
    /**
     * @brief Swap function.
     */
    friend void swap(Value& self, Value& other);

    ~Value();

    /**
     * @brief Returns the underlying variant type.
     *
     * Use this with `std::visit(lambdas, value.raw())`.
     */
    auto raw() -> Type&;
    /**
     * @brief Returns the underlying variant type.
     *
     * Use this with `std::visit(lambdas, value.raw())`.
     */
    [[nodiscard]] auto raw() const -> const Type&;

    /**
     * @brief Returns the Value as a literal string that can be directly
     * inserted in lua code.
     *
     * Throws a `std::runtime_error` if the value is a `Function`.
     */
    [[nodiscard]] auto to_literal() const -> std::string;

    /**
     * @brief Checks if the value is a string a valid identifier.
     */
    [[nodiscard]] auto is_valid_identifier() const -> bool;

    /**
     * @brief Check if the value is Nil.
     */
    [[nodiscard]] auto is_nil() const -> bool;
    /**
     * @brief Check if the value is a Bool.
     */
    [[nodiscard]] auto is_bool() const -> bool;
    /**
     * @brief Check if the value is a Number.
     */
    [[nodiscard]] auto is_number() const -> bool;
    /**
     * @brief Check if the value is a String.
     */
    [[nodiscard]] auto is_string() const -> bool;
    /**
     * @brief Check if the value is a Table.
     */
    [[nodiscard]] auto is_table() const -> bool;
    /**
     * @brief Check if the value is a Function.
     */
    [[nodiscard]] auto is_function() const -> bool;

    /**
     * @brief Check if the value has an Origin.
     */
    [[nodiscard]] auto has_origin() const -> bool;

    /**
     * @brief Return the Origin.
     */
    [[nodiscard]] auto origin() const -> const Origin&;
    [[nodiscard]] auto origin() -> Origin&;

    /**
     * @brief Remove the Origin (i.e. set it to NoOrigin).
     *
     * This is a builder style method and creates a new Value.
     */
    [[nodiscard]] auto remove_origin() const -> Value;
    /**
     * @brief Sets the Origin.
     *
     * This is a builder style method and creates a new Value.
     */
    [[nodiscard]] auto with_origin(Origin new_origin) const -> Value;
    /**
     * @brief The type of this value as a string.
     */
    [[nodiscard]] auto type() const -> std::string;

    /**
     * @brief Tries to force this value to become `new_value`.
     *
     * Does not actually change the value. This will only return a
     * `SourceChange` that (when applied) would result in the this value being
     * changed to `new_value`.
     *
     * The return value should be returned in `Function`s otherwise this does
     * not have any effect.
     *
     * This throws an exception if the types of the values didn't match.
     */
    [[nodiscard]] auto force(Value new_value, std::string origin = "") const
        -> std::optional<SourceChangeTree>;

    /**
     * @brief Call the value with the given CallContext.
     *
     * The CallContext should already contain the arguments.
     *
     * If this value is not a function or a callable table the method will throw
     * an exception.
     */
    [[nodiscard]] auto call(CallContext) const -> CallResult;
    /**
     * @brief Prepare to call the value.
     *
     * Binds the CallContext and returns a function that can be called with a
     * Vallist.
     *
     * If this value is not a function or a callable table the method will throw
     * an exception.
     */
    [[nodiscard]] auto bind(CallContext) const -> std::function<CallResult(Vallist)>;

    /**
     * @brief Call the value with the given CallContext and the args.
     *
     * The arguments will be added to the CallContext before calling the
     * function.
     *
     * If this value is not a function or a callable table the method will throw
     * an exception.
     */
    template <typename... Args>
    [[nodiscard]] auto call(const CallContext& ctx, Args... args) const {
        return this->call(ctx.make_new({args...}));
    }

    /**
     * @brief Access the value of a Table.
     *
     * If the value is not a Table this throws an exception.
     */
    auto operator[](const Value&) -> Value&;
    /**
     * @brief Access the value of a Table.
     *
     * If the value is not a Table this throws an exception.
     */
    auto operator[](const Value&) const -> const Value&;

    /**
     * @brief Convertes the value to a bool according to the lua rules.
     *
     * `Nil` and `false` is converted to `false`. Everything else is converted
     * to true.
     */
    explicit operator bool() const;

    /**
     * @brief Converts the value to a `Number`.
     *
     * If the value is already a number we return it. If it is a string we try
     * to parse it. In all other cases and if parsing fails we return `Nil`.
     *
     * If you provide a `base` the value has to be a string representing an
     * integer in that base. Otherwise `Nil` is returned.
     */
    [[nodiscard]] auto
    to_number(Value base = Nil(), std::optional<Range> location = std::nullopt) const -> Value;

    /**
     * @brief Converts the value to a `String`.
     */
    [[nodiscard]] auto to_string(std::optional<Range> location = std::nullopt) const -> Value;

    /*
     * @name Source location tracking versions of the c++ operators.
     *
     * Mostly for use in the interpreter.
     *
     * @{
     */

    /**
     * @brief unary `-` operator
     */
    [[nodiscard]] auto negate(std::optional<Range> location = std::nullopt) const -> Value;
    /**
     * @brief binary `+` operator
     */
    [[nodiscard]] auto add(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief binary `-` operator
     */
    [[nodiscard]] auto sub(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief binary `*` operator
     */
    [[nodiscard]] auto mul(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief binary `/` operator
     */
    [[nodiscard]] auto div(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief integer division
     */
    [[nodiscard]] auto int_div(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief binary `^` operator
     */
    [[nodiscard]] auto pow(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief binary `%` operator
     */
    [[nodiscard]] auto mod(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    // We can't give these methods propert names because C++ has alternate operator tokens
    // In particular using bitand, bitor, and, or and not is illegal syntax
    /**
     * @brief binary `&` operator
     */
    [[nodiscard]] auto bit_and(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief binary `|` operator
     */
    [[nodiscard]] auto bit_or(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief Bitwise xor.
     */
    [[nodiscard]] auto bit_xor(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief Bitwise shift left.
     */
    [[nodiscard]] auto bit_shl(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief Bitwise shift right.
     */
    [[nodiscard]] auto bit_shr(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief Bitwise `not` operator.
     */
    [[nodiscard]] auto bit_not(std::optional<Range> location = std::nullopt) const -> Value;

    /**
     * @brief binary `and` operator
     */
    [[nodiscard]] auto
    logic_and(const Value& rhs, std::optional<Range> location = std::nullopt) const -> Value;
    /**
     * @brief binary `-` operator
     */
    [[nodiscard]] auto
    logic_or(const Value& rhs, std::optional<Range> location = std::nullopt) const -> Value;
    /**
     * @brief unary `not` operator
     */
    [[nodiscard]] auto invert(std::optional<Range> location = std::nullopt) const -> Value;
    /**
     * @brief unary `#` operator
     */
    [[nodiscard]] auto len(std::optional<Range> location = std::nullopt) const -> Value;
    /**
     * @brief binary `==` operator
     */
    [[nodiscard]] auto equals(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief binary `!=` operator
     */
    [[nodiscard]] auto
    unequals(const Value& rhs, std::optional<Range> location = std::nullopt) const -> Value;
    /**
     * @brief binary `<` operator
     */
    [[nodiscard]] auto
    less_than(const Value& rhs, std::optional<Range> location = std::nullopt) const -> Value;
    /**
     * @brief binary `<=` operator
     */
    [[nodiscard]] auto
    less_than_or_equal(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief binary `>` operator
     */
    [[nodiscard]] auto
    greater_than(const Value& rhs, std::optional<Range> location = std::nullopt) const -> Value;
    /**
     * @brief binary `>=` operator
     */
    [[nodiscard]] auto
    greater_than_or_equal(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /**
     * @brief binary `..` operator
     */
    [[nodiscard]] auto concat(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    /** @} */

    friend auto operator-(const Value&) -> Value;
    friend auto operator+(const Value&, const Value&) -> Value;
    friend auto operator-(const Value&, const Value&) -> Value;
    friend auto operator*(const Value&, const Value&) -> Value;
    friend auto operator/(const Value&, const Value&) -> Value;
    friend auto operator^(const Value&, const Value&) -> Value;
    friend auto operator%(const Value&, const Value&) -> Value;
    friend auto operator&(const Value&, const Value&) -> Value;
    friend auto operator|(const Value&, const Value&) -> Value;
    friend auto operator&&(const Value&, const Value&) -> Value;
    friend auto operator||(const Value&, const Value&) -> Value;
    friend auto operator!(const Value&) -> Value;
};

auto operator==(const Value&, const Value&) noexcept -> bool;
auto operator!=(const Value&, const Value&) noexcept -> bool;
auto operator<<(std::ostream&, const Value&) -> std::ostream&;

} // namespace minilua

namespace std {

/**
 * Behaves like `std::get(std::variant)` but only accepts types as template parameter.
 */
template <typename T> auto get(minilua::Value& value) -> T& { return std::get<T>(value.raw()); }
/**
 * Behaves like `std::get(const std::variant&)` but only accepts types as template parameter.
 */
template <typename T> auto get(const minilua::Value& value) -> const T& {
    return std::get<T>(value.raw());
}

} // namespace std

namespace minilua {

inline auto _ignore_nan_and_infinity(std::optional<Number> value) -> std::optional<Number> {
    if (value.has_value()) {
        return value.value().visit(overloaded{
            [](Number::Int value) -> std::optional<Number> { return value; },
            [](Number::Float value) -> std::optional<Number> {
                if (std::isnan(value) || std::isinf(value)) {
                    return std::nullopt;
                } else {
                    return value;
                }
            },
        });
    } else {
        return std::nullopt;
    }
}

/*
 * Helper functions for writing functions that should be forcable.
 */

/**
 * @brief Helper to create the reverse function for `UnaryOrigin` or
 * `UnaryNumericFunctionHelper`.
 *
 * You only need to supply a function that accepts one `Number` (the new value for the result)
 * and returns `Number` (the new value for the parameter).
 *
 * \note Don't use this if the reverse can fail.
 *
 * See also: @ref binary_num_reverse.
 */
template <typename Fn> auto unary_num_reverse(Fn fn) -> decltype(auto) {
    static_assert(std::is_convertible_v<std::invoke_result_t<Fn, Number>, std::optional<Number>>);

    return [fn](const Value& new_value, const Value& old_value) -> std::optional<SourceChangeTree> {
        if (!new_value.is_number() || !old_value.is_number()) {
            return std::nullopt;
        }

        auto num = std::get<Number>(new_value);

        std::optional<Number> reverse_value = _ignore_nan_and_infinity(fn(num));
        if (reverse_value.has_value()) {
            return old_value.force(reverse_value.value());
        } else {
            return std::nullopt;
        }
    };
}

/**
 * @brief Helper to create the reverse function for `BinaryOrigin` or
 * `BinaryNumericFunctionHelper`.
 *
 * You only need to supply a function that accepts two `Number`s (the new value and old lhs/rhs
 * value) and returns `Number` (the new value for the lhs/rhs parameter).
 *
 * \note Don't use this if the reverse can fail.
 *
 * See also: @ref unary_num_reverse.
 */
template <typename FnLeft, typename FnRight>
auto binary_num_reverse(FnLeft fn_left, FnRight fn_right, std::string origin = "")
    -> decltype(auto) {
    static_assert(
        std::is_convertible_v<std::invoke_result_t<FnLeft, Number, Number>, std::optional<Number>>);
    static_assert(std::is_convertible_v<
                  std::invoke_result_t<FnRight, Number, Number>, std::optional<Number>>);

    return [fn_left, fn_right, origin = std::move(origin)](
               const Value& new_value, const Value& old_lhs,
               const Value& old_rhs) -> std::optional<SourceChangeTree> {
        if (!new_value.is_number() || !old_lhs.is_number() || !old_rhs.is_number()) {
            return std::nullopt;
        }
        auto num = std::get<Number>(new_value);
        auto lhs_num = std::get<Number>(old_lhs);
        auto rhs_num = std::get<Number>(old_rhs);

        SourceChangeAlternative change;

        std::optional<Number> reverse_left_result = _ignore_nan_and_infinity(fn_left(num, rhs_num));
        if (reverse_left_result.has_value()) {
            std::optional<SourceChangeTree> lhs_change = old_lhs.force(reverse_left_result.value());
            change.add_if_some(lhs_change);
        }

        std::optional<Number> reverse_right_result =
            _ignore_nan_and_infinity(fn_right(num, lhs_num));
        if (reverse_right_result.has_value()) {
            std::optional<SourceChangeTree> rhs_change =
                old_rhs.force(reverse_right_result.value());
            change.add_if_some(rhs_change);
        }

        change.origin = origin;
        return change;
    };
}

/**
 * @brief Helper for creating reversible numeric unary function.
 *
 * \note Don't use this if the reverse can fail.
 *
 * With this helper you don't have to manually match the `Value`s you just have to
 * provide functions that take `Number` values and return `Number` values for:
 *
 * 1. the normal function you want to write
 * 2. the reverse function for propagating the changed result to the parameter
 *
 * Usage (using the deduction guide):
 *
 * ```cpp
 * function sqrt_impl(const CallContext& ctx) -> Value {
 *     return minilua::UnaryNumericFunctionHelper{
 *         [](Number param) { return std::sqrt(param.as_float()); },
 *         [](Number new_value, Number old_param) { return new_value.as_float() *
 * new_value.as_float(); },
 *      }(ctx);
 * }
 * ```
 *
 * Note the call at the end.
 *
 * It's also possible to directly use the `UnaryNumericFunctionHelper` as the
 * argument to `Function` because it is callable with a `CallContext` argument.
 *
 * See also: `BinaryNumericFunctionHelper`.
 */
template <typename Fn, typename Reverse> struct UnaryNumericFunctionHelper {
    Fn function;
    Reverse reverse;

    auto operator()(const CallContext& ctx) -> Value {
        auto [arg1, origin] = ctx.unary_numeric_arg_helper();
        origin.reverse = unary_num_reverse(this->reverse);
        return Value(this->function(arg1)).with_origin(origin);
    }
};
/** deduction guide */
template <typename... Ts> UnaryNumericFunctionHelper(Ts...) -> UnaryNumericFunctionHelper<Ts...>;

/**
 * @brief Helper for creating a reversible numeric binary function.
 *
 * \note Don't use this if the reverse can fail.
 *
 * With this helper you don't have to manually match the `Value`s you just have to
 * provide functions that take `Number` values and return `Number` values for:
 *
 * 1. the normal function you want to write
 * 2. the reverse function for propagating the changed result to the left parameter
 * 3. the reverse function for propagating the changed result to the right parameter
 *
 * Usage (using the deduction guide):
 *
 * ```cpp
 * function pow_impl(const CallContext& ctx) -> Value {
 *     return minilua::BinaryNumericFunctionHelper{
 *         [](Number lhs, Number rhs) { return std::pow(lhs.as_float(), rhs.as_float()); },
 *         [](Number new_value, Number old_rhs) { return std::pow(new_value.as_float(), 1 /
 * old_rhs.as_float()); },
 *         [](Number new_value, Number old_lhs) { return std::log(new_value.as_float()) /
 * std::log(old_lhs.as_float()); }
 *      }(ctx);
 * }
 * ```
 *
 * Note the call at the end.
 *
 * It's also possible to directly use the `BinaryNumericFunctionHelper` as the
 * argument to `Function` because it is callable with a `CallContext` argument.
 *
 * See also: `UnaryNumericFunctionHelper`.
 */
template <typename Fn, typename ReverseLeft, typename ReverseRight>
struct BinaryNumericFunctionHelper {
    Fn function;
    ReverseLeft reverse_left;
    ReverseRight reverse_right;

    auto operator()(const CallContext& ctx) -> Value {
        auto [arg1, arg2, origin] = ctx.binary_numeric_args_helper();
        origin.reverse = binary_num_reverse(this->reverse_left, this->reverse_right);
        return Value(this->function(arg1, arg2)).with_origin(origin);
    }
};
/** deduction guide */
template <class... Ts> BinaryNumericFunctionHelper(Ts...) -> BinaryNumericFunctionHelper<Ts...>;

// helper functions

/**
 * @brief Parse a string into a lua value number.
 */
auto parse_number_literal(const std::string& str) -> Value;
/**
 * @brief Parse and escape a string into a lua value string.
 */
auto parse_string_literal(const std::string& str) -> Value;

} // namespace minilua

#endif
