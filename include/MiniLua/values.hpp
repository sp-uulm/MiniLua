#ifndef MINILUA_VALUES_HPP
#define MINILUA_VALUES_HPP

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
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
 * A vallist can contain an arbitrary amount of 'Value's.
 *
 * You can use a Vallist in destructuring assignments. You have to specify the
 * number of values you want. If the number is lower than the amount of values
 * it will simply return the first values. If the number is higher than the
 * amount of values it will return references to a `Nil` value for the remaining
 * values.
 *
 * NOTE: This will actually return 'std::reference_wrapper's because
 * it's not possible to put references inside a tuple. That means that you have
 * to call `get` on the values to use them:
 *
 * ```
 * auto& [one, two, three] = vallist.tuple<3>();
 * out.get();
 * two.get();
 * ```
 *
 * You can iterate over a Vallist and you can get one element by index using `Vallist::get`.
 * Iteration only works for the exact number of elements (like for `std::vector`),
 * so you need to be carful not to dereference a pointer into an empty Vallist).
 * `get` will return `Nil` values if the element you request is not in the Vallist.
 */
class Vallist {
    struct Impl;
    owning_ptr<Impl> impl;

public:
    Vallist();
    explicit Vallist(Value);
    Vallist(std::vector<Value>);
    Vallist(std::initializer_list<Value>);
    // concatenate vallists
    Vallist(std::vector<Vallist>);

    Vallist(const Vallist&);
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    Vallist(Vallist&&);
    auto operator=(const Vallist&) -> Vallist&;
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    auto operator=(Vallist&&) -> Vallist&;
    ~Vallist();

    /**
     * Returns the number of actual Values in the Vallist.
     */
    [[nodiscard]] auto size() const -> size_t;

    /**
     * Returns the value at the given index.
     *
     * If the value does not exist a reference to a Nil value will be returned.
     */
    [[nodiscard]] auto get(size_t index) const -> const Value&;

    /**
     * Iterator over the Vallist. Can be used in e.g. for loops.
     */
    [[nodiscard]] auto begin() const -> std::vector<Value>::const_iterator;
    [[nodiscard]] auto end() const -> std::vector<Value>::const_iterator;

    // helper for next method
    template <std::size_t... Is>
    [[nodiscard]] auto tuple(std::index_sequence<Is...> /*unused*/) const
        -> std::tuple<T_<Is, std::reference_wrapper<const Value>>...> {
        return std::make_tuple(std::cref(this->get(Is))...);
    }

    /**
     * Will return the given number of values in a tuple.
     *
     * If the given number is bigger than the number of values the remaining items
     * in the tuple will be filled with Nil.
     *
     * Use this for structured binding declarations:
     *
     * ```
     * const auto& [val1, val2, val3] = vallist.tuple<3>();
     * ```
     *
     * NOTE: The values will be `std::reference_wrapper`s becuase it's not
     * possible to put references in a tuple. You have to call `get` on the
     * values before using them.
     */
    template <std::size_t N> [[nodiscard]] auto tuple() const {
        return tuple(std::make_index_sequence<N>{});
    }

    friend auto operator==(const Vallist&, const Vallist&) -> bool;
    friend auto operator<<(std::ostream&, const Vallist&) -> std::ostream&;
};

struct Nil {
    constexpr static const std::string_view TYPE = "nil";

    [[nodiscard]] auto to_literal() const -> std::string;

    explicit operator bool() const;
};
constexpr auto operator==(Nil /*unused*/, Nil /*unused*/) noexcept -> bool { return true; }
constexpr auto operator!=(Nil /*unused*/, Nil /*unused*/) noexcept -> bool { return false; }
auto operator<<(std::ostream&, Nil) -> std::ostream&;

struct Bool {
    bool value; // NOLINT(misc-non-private-member-variables-in-classes)

    constexpr static const std::string_view TYPE = "boolean";

    constexpr Bool(bool value) : value(value) {}

    [[nodiscard]] auto to_literal() const -> std::string;

    explicit operator bool() const;
};
constexpr auto operator==(Bool lhs, Bool rhs) noexcept -> bool { return lhs.value == rhs.value; }
constexpr auto operator!=(Bool lhs, Bool rhs) noexcept -> bool { return !(lhs == rhs); }
auto operator<<(std::ostream&, Bool) -> std::ostream&;

// normal c++ operators
DELEGATE_OP(Bool, &&);
DELEGATE_OP(Bool, ||);
DELEGATE_OP(Bool, ^);

struct Number {
    double value; // NOLINT(misc-non-private-member-variables-in-classes)

    constexpr static const std::string_view TYPE = "number";

    constexpr Number(int value) : value(value) {}
    constexpr Number(double value) : value(value) {}

    [[nodiscard]] auto to_literal() const -> std::string;

    explicit operator bool() const;

    [[nodiscard]] auto is_int() const -> bool;
};
constexpr auto operator==(Number lhs, Number rhs) noexcept -> bool {
    return lhs.value == rhs.value;
}
constexpr auto operator!=(Number lhs, Number rhs) noexcept -> bool { return !(lhs == rhs); }
constexpr auto operator<(Number lhs, Number rhs) noexcept -> bool { return lhs.value < rhs.value; }
constexpr auto operator>(Number lhs, Number rhs) noexcept -> bool { return lhs.value > rhs.value; }
constexpr auto operator<=(Number lhs, Number rhs) noexcept -> bool {
    return lhs.value <= rhs.value;
}
constexpr auto operator>=(Number lhs, Number rhs) noexcept -> bool {
    return lhs.value >= rhs.value;
}
auto operator<<(std::ostream&, Number) -> std::ostream&;

// normal c++ operators
auto operator-(Number self) -> Number;
DELEGATE_OP(Number, +);
DELEGATE_OP(Number, -);
DELEGATE_OP(Number, *);
DELEGATE_OP(Number, /);
// exponentiation (pow)
auto operator^(Number lhs, Number rhs) -> Number;
auto operator%(Number lhs, Number rhs) -> Number;
auto operator&(Number lhs, Number rhs) -> Number;
auto operator|(Number lhs, Number rhs) -> Number;

struct String {
    std::string value; // NOLINT(misc-non-private-member-variables-in-classes)

    constexpr static const std::string_view TYPE = "string";

    String(std::string value);

    [[nodiscard]] auto to_literal() const -> std::string;

    [[nodiscard]] auto is_valid_identifier() const -> bool;

    explicit operator bool() const;

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
 * Table is basically a `std::map`.
 *
 * Aditionally table is aliasable. That means two variables (or two `Table` `Value`s)
 * can refer to the same actual table.
 *
 * NOTE: With the current implementation this can lead to memory leaks if you create
 * a cycle. I.e. a field of the table references (directly or indirectly) the table itself.
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

    constexpr static const std::string_view TYPE = "table";

    // NOTE: default constructor has to be separate from the one only taking the
    // allocator and we can't use default arguments there.
    Table();
    Table(MemoryAllocator* allocator);
    Table(std::unordered_map<Value, Value>, MemoryAllocator* allocator = &GLOBAL_ALLOCATOR);
    Table(
        std::initializer_list<std::pair<const Value, Value>> values,
        MemoryAllocator* allocator = &GLOBAL_ALLOCATOR);

    /**
     * Copy table to different allocator.
     *
     * This will make a deep copy meaning all nested tables will also be copied
     * to the allocator.
     */
    Table(const Table& other, MemoryAllocator* allocator);

    Table(const Table& other);
    Table(Table&& other) noexcept;
    ~Table() noexcept;
    auto operator=(const Table& other) -> Table&;
    auto operator=(Table&& other) noexcept -> Table&;
    friend void swap(Table& self, Table& other);

    /**
     * The result of the lua length operator `#`.
     *
     * Satisfies: `(border == 0 or t[border] ~= nil) and t[border + 1] == nil`
     */
    [[nodiscard]] auto border() const -> int;

    auto get(const Value& key) -> Value;
    auto has(const Value& key) -> bool;
    void set(const Value& key, Value value);
    void set(Value&& key, Value value);
    /**
     * Copy the `other` table into this table overwriting all keys that are
     * duplicate.
     */
    void set_all(const Table& other);
    [[nodiscard]] auto size() const -> size_t;

    // iterators for Table
    auto begin() -> iterator;
    [[nodiscard]] auto begin() const -> const_iterator;
    [[nodiscard]] auto cbegin() const -> const_iterator;
    auto end() -> iterator;
    [[nodiscard]] auto end() const -> const_iterator;
    [[nodiscard]] auto cend() const -> const_iterator;

    [[nodiscard]] auto to_literal() const -> std::string;

    /**
     * Next returns the next index of the table and its associated value after index.
     *
     * If there is no value at the index an exception is thrown.
     *
     * @param key The index you want to get the next element.
     *
     * @return An empty `Vallist` when called with the last index or on an empty
     * table. Else it returns the next index and its associated value.
     */
    [[nodiscard]] auto next(const Value& key) const -> Vallist;

    friend auto operator==(const Table&, const Table&) noexcept -> bool;
    friend auto operator!=(const Table&, const Table&) noexcept -> bool;
    friend auto operator<<(std::ostream&, const Table&) -> std::ostream&;

    friend struct std::hash<Table>;

    // TODO maybe return proxy "entry" type to avoid unnecessary Nil values
    auto operator[](const Value&) -> Value&;
    auto operator[](const Value&) const -> const Value&;

    explicit operator bool() const;
};

struct BinaryOrigin;
struct UnaryOrigin;

/**
 * Contains information for use in the implementation of native functions.
 *
 * Contains the arguments and the environment.
 *
 * The `CallContext` and the `Environment` should not be copied and stored
 * somewhere that outlives the function call. Because the environment may be
 * freed after the call to the function ends. You can however safely store any
 * `Value`s or a copy of the arguments.
 */
class CallContext {
    struct Impl;
    owning_ptr<Impl> impl;

public:
    CallContext(Environment* env);
    CallContext(const CallContext&);
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    CallContext(CallContext&&);
    auto operator=(const CallContext&) -> CallContext&;
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    auto operator=(CallContext&&) -> CallContext&;
    ~CallContext();

    /**
     * Create a new call context with new arguments but reuse all other information.
     */
    [[nodiscard]] auto make_new(Vallist, std::optional<Range> location = std::nullopt) const
        -> CallContext;

    template <typename... Args>
    [[nodiscard]] auto make_new(Args... args, std::optional<Range> location = std::nullopt) const
        -> CallContext {
        return this->make_new(Vallist{args...}, location);
    }

    /**
     * Helper method to create a table in the allocator of the environment.
     */
    [[nodiscard]] auto make_table() const -> Table;

    /**
     * Returns the location of the call.
     */
    [[nodiscard]] auto call_location() const -> std::optional<Range>;

    /**
     * Returns a reference to the environment.
     *
     * For `Function`s created in C++ you can only access the global
     * environment.
     *
     * For functions created in lua you can access the global environment and
     * local variables that were in scope when creating the function.
     */
    [[nodiscard]] auto environment() const -> Environment&;

    /**
     * Returns the value of a variable accessible from the function.
     *
     * Returns `Nil` if the variable is not accessible or does not exist.
     */
    [[nodiscard]] auto get(const std::string& name) const -> Value;

    /**
     * Returns the arguments given to this function.
     */
    [[nodiscard]] auto arguments() const -> const Vallist&;

    /**
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
    [[nodiscard]] auto unary_numeric_arg_helper() const -> std::tuple<double, UnaryOrigin>;
    /**
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
        -> std::tuple<double, double, BinaryOrigin>;

    friend auto operator<<(std::ostream&, const CallContext&) -> std::ostream&;
};

/**
 * Return value of the implementation of native function.
 *
 * Contains the actual return value and optionally source changes.
 */
class CallResult {
    Vallist vallist;
    std::optional<SourceChangeTree> _source_change;

public:
    CallResult();
    CallResult(Vallist);
    CallResult(std::vector<Value>);
    explicit CallResult(std::initializer_list<Value>);
    explicit CallResult(SourceChangeTree);
    explicit CallResult(std::optional<SourceChangeTree>);
    explicit CallResult(Vallist, SourceChangeTree);
    explicit CallResult(Vallist, std::optional<SourceChangeTree>);

    /**
     * Get the return values.
     */
    [[nodiscard]] auto values() const -> const Vallist&;
    /**
     * Get the source change.
     */
    [[nodiscard]] auto source_change() const -> const std::optional<SourceChangeTree>&;
};

auto operator==(const CallResult&, const CallResult&) -> bool;
auto operator<<(std::ostream&, const CallResult&) -> std::ostream&;

/**
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
 * function (e.g. `if`, `while`, ...). Because this might break the source change
 * mechanism if you use operators on `Value`s. If you use control flow constructs
 * you should remove the origin from the returned Value with `Value::remove_origin`.
 */
class Function {
    using FnType = CallResult(CallContext);

    std::shared_ptr<std::function<FnType>> func;
    std::string name;

public:
    constexpr static const std::string_view TYPE = "function";

    template <typename Fn, typename = std::enable_if_t<std::is_invocable_v<Fn, CallContext>>>
    Function(Fn fn) : Function(fn, "") {}

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

    // always throws an exception. just here for convenience.
    [[nodiscard]] auto to_literal() const -> std::string;

    [[nodiscard]] auto call(CallContext) const -> CallResult;

    explicit operator bool() const;

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
 * Default origin for `Value`s.
 */
struct NoOrigin {};
auto operator==(const NoOrigin&, const NoOrigin&) noexcept -> bool;
auto operator!=(const NoOrigin&, const NoOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const NoOrigin&) -> std::ostream&;

/**
 * Can be used for externally produced values but is mainly a placeholder.
 *
 * TODO future use could include allowing to specify a function to change the
 * external value (like it is possible for values produced from literals).
 */
struct ExternalOrigin {};
auto operator==(const ExternalOrigin&, const ExternalOrigin&) noexcept -> bool;
auto operator!=(const ExternalOrigin&, const ExternalOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const ExternalOrigin&) -> std::ostream&;

/**
 * Origin for a Value that was created from a literal in code.
 */
struct LiteralOrigin {
    Range location;
};

auto operator==(const LiteralOrigin&, const LiteralOrigin&) noexcept -> bool;
auto operator!=(const LiteralOrigin&, const LiteralOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const LiteralOrigin&) -> std::ostream&;

/**
 * Origin for a Value that was created in a binary operation
 * (or some functions with two arguments) using lhs and rhs.
 */
struct BinaryOrigin {
    using ReverseFn = std::optional<SourceChangeTree>(const Value&, const Value&, const Value&);

    owning_ptr<Value> lhs;
    owning_ptr<Value> rhs;
    std::optional<Range> location;
    // new_value, old_lhs, old_rhs
    std::function<ReverseFn> reverse;
};

auto operator==(const BinaryOrigin&, const BinaryOrigin&) noexcept -> bool;
auto operator!=(const BinaryOrigin&, const BinaryOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const BinaryOrigin&) -> std::ostream&;

/**
 * Origin for a Value that was created in a unary operation
 * (or some functions with one argument) using val.
 */
struct UnaryOrigin {
    using ReverseFn = std::optional<SourceChangeTree>(const Value&, const Value&);

    owning_ptr<Value> val;
    std::optional<Range> location;
    // new_value, old_value
    std::function<ReverseFn> reverse;
};

auto operator==(const UnaryOrigin&, const UnaryOrigin&) noexcept -> bool;
auto operator!=(const UnaryOrigin&, const UnaryOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const UnaryOrigin&) -> std::ostream&;

/**
 * The origin of a value.
 *
 * Defaults to `NoOrigin`.
 *
 * Using `BinaryOrigin` and `UnaryOrigin` this build a tree to track the changes
 * made to a Value while running a lua program.
 *
 * You can manually walk the tree using the variant returned by `raw`.
 */
class Origin {
public:
    using Type = std::variant<NoOrigin, ExternalOrigin, LiteralOrigin, BinaryOrigin, UnaryOrigin>;

private:
    Type origin;

public:
    Origin();
    explicit Origin(Type);
    Origin(NoOrigin);
    Origin(ExternalOrigin);
    Origin(LiteralOrigin);
    Origin(BinaryOrigin);
    Origin(UnaryOrigin);

    [[nodiscard]] auto raw() const -> const Type&;
    auto raw() -> Type&;

    [[nodiscard]] auto is_none() const -> bool;
    [[nodiscard]] auto is_external() const -> bool;
    [[nodiscard]] auto is_literal() const -> bool;
    [[nodiscard]] auto is_binary() const -> bool;
    [[nodiscard]] auto is_unary() const -> bool;

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

namespace std {

/**
 * Behaves like `std::get(std::variant)` but only accepts types as template parameter.
 */
template <typename T> auto get(minilua::Origin& origin) -> T& { return std::get<T>(origin.raw()); }
/**
 * Behaves like `std::get(std::variant)` but only accepts types as template parameter.
 */
template <typename T> auto get(const minilua::Origin& origin) -> const T& {
    return std::get<T>(origin.raw());
}

} // namespace std

namespace minilua {

/**
 * Represents a value in lua.
 *
 * You can use most normal C++ operators on these value (`+`, `-`, `*`, `/`, `[]`, ...). If the
 * operation can't be performed on the actual value type an exception will be thrown. Logical
 * operators are implemented like they work in lua (i.e. `"hi" && true == "hi"`).
 *
 * There are also variants of all the operators that track the origin of the values.
 *
 * You can get the underlying value either via `std::visit(lambdas, value.raw())`
 * or using `std::get<T>(value)` where `T` is any of the underlying types.
 *
 * Most values can not be changed (i.e. if you add two numbers you create a new value).
 *
 * `Function` and `Table` are different in that they act like reference (or act like they are
 * behind a `std::shared_ptr`). I.e. two variables can refer to the same table and function.
 * In the case of function this is irrelevant because (from the outside) they are immutable.
 * However tables can be mutated from multiple variables.
 *
 * NOTE: The current implementation of Table can lead to a memory leak if you create cycles with
 * them. I.e. a field in the table refers (directly or indirectly) to the table that owns it.
 */
class Value {
    struct Impl;
    owning_ptr<Impl> impl;

public:
    using Type = std::variant<Nil, Bool, Number, String, Table, Function>;

    Value();
    Value(Type val);
    Value(Nil val);
    Value(Bool val);
    Value(bool val);
    Value(Number val);
    Value(int val);
    Value(double val);
    Value(String val);
    Value(std::string val);
    Value(const char* val);
    Value(Table val);
    Value(Function val);

    /**
     * Overloaded constructor usable with references to lambdas and functions.
     *
     * NOTE: Functions with a parameter of `CallContext&` does not work.
     * Only `CallContext` and `const CallContext&`.
     */
    template <typename Fn, typename = std::enable_if_t<std::is_invocable_v<Fn, CallContext>>>
    Value(Fn val) : Value(Function(std::forward<Fn>(val))) {}

    /**
     * Copies the value to another allocator.
     *
     * This only has an effect for `Table`s. Other values are simply copied.
     * Table will be deep copied to the given allocators. That means all nested
     * tables will also be copied.
     */
    Value(const Value&, MemoryAllocator* allocator);

    Value(const Value&);
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    Value(Value&&);
    auto operator=(const Value& other) -> Value&;
    // can't use noexcept = default in older compilers (pre c++20 compilers)
    // NOLINTNEXTLINE
    auto operator=(Value&& other) -> Value&;
    friend void swap(Value& self, Value& other);

    ~Value();

    /**
     * Use as `std::visit(lambdas, value.raw())`.
     */
    auto raw() -> Type&;
    [[nodiscard]] auto raw() const -> const Type&;

    /**
     * Returns the Value as a literal string that can be directly inserted in lua code.
     *
     * Throws a `std::runtime_error` if the value is a `Function`.
     */
    [[nodiscard]] auto to_literal() const -> std::string;

    /**
     * Checks if the value is a string a valid identifier.
     */
    [[nodiscard]] auto is_valid_identifier() const -> bool;

    [[nodiscard]] auto is_nil() const -> bool;
    [[nodiscard]] auto is_bool() const -> bool;
    [[nodiscard]] auto is_number() const -> bool;
    [[nodiscard]] auto is_string() const -> bool;
    [[nodiscard]] auto is_table() const -> bool;
    [[nodiscard]] auto is_function() const -> bool;

    [[nodiscard]] auto has_origin() const -> bool;

    [[nodiscard]] auto origin() const -> const Origin&;
    [[nodiscard]] auto origin() -> Origin&;

    [[nodiscard]] auto remove_origin() const -> Value;
    [[nodiscard]] auto with_origin(Origin new_origin) const -> Value;
    [[nodiscard]] auto type() const -> std::string;

    /**
     * Tries to force this value to become `new_value`. Does not actually change
     * the value. This will only return a `SourceChange` that (when applied) would
     * result in the this value being changed to `new_value`.
     *
     * The return value should be returned in `Function`s otherwise this
     * does not have any effect.
     *
     * This throws an exception if the types of the values didn't match.
     */
    [[nodiscard]] auto force(Value new_value, std::string origin = "") const
        -> std::optional<SourceChangeTree>;

    [[nodiscard]] auto call(CallContext) const -> CallResult;
    [[nodiscard]] auto bind(CallContext) const -> std::function<CallResult(Vallist)>;

    template <typename... Args>
    [[nodiscard]] auto call(const CallContext& ctx, Args... args) const {
        return this->call(ctx.make_new({args...}));
    }

    auto operator[](const Value&) -> Value&;
    auto operator[](const Value&) const -> const Value&;

    /**
     * Convertes the value to a bool according to the lua rules.
     *
     * `Nil` and `false` is converted to `false`. Everything else is converted
     * to true.
     */
    explicit operator bool() const;

    /**
     * Converts the value to a `Number`.
     *
     * If the value is already a number we return it. If it is a string we try to
     * parse it. In all other cases and if parsing fails we return `Nil`.
     *
     * If you provide a `base` the value has to be a string representing an integer
     * in that base. Otherwise `Nil` is returned.
     */
    [[nodiscard]] auto
    to_number(Value base = Nil(), std::optional<Range> location = std::nullopt) const -> Value;
    [[nodiscard]] auto to_string(std::optional<Range> location = std::nullopt) const -> Value;

    /*
     * @name Source location tracking versions of the c++ operators.
     *
     * Mostly for use in the interpreter.
     */
    /** @{ */

    /**
     * unary `-` operator
     */
    [[nodiscard]] auto negate(std::optional<Range> location = std::nullopt) const -> Value;
    [[nodiscard]] auto add(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    [[nodiscard]] auto sub(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    [[nodiscard]] auto mul(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    [[nodiscard]] auto div(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    [[nodiscard]] auto pow(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    [[nodiscard]] auto mod(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    // We can't give these methods propert names because C++ has alternate operator tokens
    // In particular using bitand, bitor, and, or and not is illegal syntax
    [[nodiscard]] auto bit_and(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    [[nodiscard]] auto bit_or(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    [[nodiscard]] auto
    logic_and(const Value& rhs, std::optional<Range> location = std::nullopt) const -> Value;
    [[nodiscard]] auto
    logic_or(const Value& rhs, std::optional<Range> location = std::nullopt) const -> Value;
    /**
     * unary `not` operator
     */
    [[nodiscard]] auto invert(std::optional<Range> location = std::nullopt) const -> Value;
    /**
     * unary `#` operator
     */
    [[nodiscard]] auto len(std::optional<Range> location = std::nullopt) const -> Value;
    [[nodiscard]] auto equals(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    [[nodiscard]] auto
    unequals(const Value& rhs, std::optional<Range> location = std::nullopt) const -> Value;
    [[nodiscard]] auto
    less_than(const Value& rhs, std::optional<Range> location = std::nullopt) const -> Value;
    [[nodiscard]] auto
    less_than_or_equal(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
    [[nodiscard]] auto
    greater_than(const Value& rhs, std::optional<Range> location = std::nullopt) const -> Value;
    [[nodiscard]] auto
    greater_than_or_equal(const Value& rhs, std::optional<Range> location = std::nullopt) const
        -> Value;
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

/*
 * Helper functions for writing functions that should be forcable.
 */

/**
 * Helper to create the reverse function for `UnaryOrigin` or `UnaryNumericFunctionHelper`.
 *
 * You only need to supply a function that accepts two `double`s (the new and old values)
 * and returns `double` (the new value for the parameter).
 *
 * NOTE: Don't use this if the reverse can fail.
 *
 * See also: @ref binary_num_reverse.
 */
template <typename Fn> auto unary_num_reverse(Fn fn) -> decltype(auto) {
    return [fn](const Value& new_value, const Value& old_value) -> std::optional<SourceChangeTree> {
        if (!new_value.is_number() || !old_value.is_number()) {
            return std::nullopt;
        }
        double num = std::get<Number>(new_value).value;
        return old_value.force(fn(num));
    };
}

/**
 * Helper to create the reverse function for `BinaryOrigin` or `BinaryNumericFunctionHelper`.
 *
 * You only need to supply a function that accepts two `double`s (the new value and old lhs/rhs
 * value) and returns `double` (the new value for the lhs/rhs parameter).
 *
 * NOTE: Don't use this if the reverse can fail.
 *
 * See also: @ref unary_num_reverse.
 */
template <typename FnLeft, typename FnRight>
auto binary_num_reverse(FnLeft fn_left, FnRight fn_right, std::string origin = "")
    -> decltype(auto) {
    return [fn_left, fn_right, origin = std::move(origin)](
               const Value& new_value, const Value& old_lhs,
               const Value& old_rhs) -> std::optional<SourceChangeTree> {
        if (!new_value.is_number() || !old_lhs.is_number() || !old_rhs.is_number()) {
            return std::nullopt;
        }
        double num = std::get<Number>(new_value).value;
        double lhs_num = std::get<Number>(old_lhs).value;
        double rhs_num = std::get<Number>(old_rhs).value;

        auto lhs_change = old_lhs.force(fn_left(num, rhs_num));
        auto rhs_change = old_rhs.force(fn_right(num, lhs_num));

        SourceChangeAlternative change;
        change.add_if_some(lhs_change);
        change.add_if_some(rhs_change);

        change.origin = origin;
        return change;
    };
}

/**
 * Helper for creating reversible numeric unary function.
 *
 * NOTE: Don't use this if the reverse can fail.
 *
 * With this helper you don't have to manually match the `Value`s you just have to
 * provide functions that take `double` values and return `double` values for:
 *
 * 1. the normal function you want to write
 * 2. the reverse function for propagating the changed result to the parameter
 *
 * Usage (using the deduction guide):
 *
 * ```
 * function sqrt_impl(const CallContext& ctx) -> Value {
 *     return minilua::UnaryNumericFunctionHelper{
 *         [](double param) { return std::sqrt(param); },
 *         [](double new_value, double old_param) { return new_value * new_value; },
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
 * Helper for creating a reversible numeric binary function.
 *
 * NOTE: Don't use this if the reverse can fail.
 *
 * With this helper you don't have to manually match the `Value`s you just have to
 * provide functions that take `double` values and return `double` values for:
 *
 * 1. the normal function you want to write
 * 2. the reverse function for propagating the changed result to the left parameter
 * 3. the reverse function for propagating the changed result to the right parameter
 *
 * Usage (using the deduction guide):
 *
 * ```
 * function pow_impl(const CallContext& ctx) -> Value {
 *     return minilua::BinaryNumericFunctionHelper{
 *         [](double lhs, double rhs) { return std::pow(lhs, rhs); },
 *         [](double new_value, double old_rhs) { return std::pow(new_value, 1 / old_rhs); },
 *         [](double new_value, double old_lhs) { return std::log(new_value) / std::log(old_lhs); }
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
 * Parse a string into a lua value number.
 */
auto parse_number_literal(const std::string& str) -> Value;
/**
 * Parse and escape a string into a lua value string.
 */
auto parse_string_literal(const std::string& str) -> Value;

} // namespace minilua

#endif
