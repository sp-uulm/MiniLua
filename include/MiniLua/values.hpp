#ifndef MINILUA_VALUES_HPP
#define MINILUA_VALUES_HPP

#include <cmath>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "environment.hpp"
#include "source_change.hpp"
#include "utils.hpp"

// helper macros to delegate a binary operator
#define DELEGATE_OP(TYPE, OP)                                                                      \
    constexpr auto operator OP(const TYPE& lhs, const TYPE& rhs)->TYPE {                           \
        return TYPE(lhs.value OP rhs.value);                                                       \
    }

namespace minilua {

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
 * amount of values it will return references to a Nil value for the remaining
 * values. Note: This will actually return 'std::reference_wrapper's because
 * it's not possible to put references inside a tuple. That means that you have
 * to call 'get' on the values.
 *
 * ```
 * auto& [one, two, three] = vallist.tuple<3>();
 * out.get();
 * two.get();
 * ```
 *
 * You can iterate over a Vallist and you can get one element by index using 'get'.
 */
class Vallist {
    struct Impl;
    owning_ptr<Impl> impl;

public:
    Vallist();
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
     * NOTE: The values will be 'std::reference_wrapper's becuase it's not
     * possible to put references in a tuple. You have to call 'get' on the
     * values before using it.
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
auto operator<<(std::ostream&, const String&) -> std::ostream&;

// Forward declaration
class CallContext;

class Table {
    struct Impl;
    std::shared_ptr<Impl> impl;

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

    Table();
    Table(std::unordered_map<Value, Value>);
    Table(std::initializer_list<std::pair<const Value, Value>> values);

    Table(const Table& other);
    Table(Table&& other) noexcept;
    ~Table() noexcept;
    auto operator=(const Table& other) -> Table&;
    auto operator=(Table&& other) noexcept -> Table&;
    friend void swap(Table& self, Table& other);

    auto get(const Value& key) -> Value;
    auto has(const Value& key) -> bool;
    void set(const Value& key, Value value);
    void set(Value&& key, Value value);
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
     * @brief next returns the next index of the table and its associated value after index. If
     * there is no value at the index an error is thrown.
     * @param key is the index you want to get the next element. Key is the index
     * @return nil when called with the last index or an the empty table. Else it returns the next
     * index and its associated value after the value at index.
     */
    [[nodiscard]] auto next(const Value& key) const -> Vallist;

    friend auto operator==(const Table&, const Table&) noexcept -> bool;
    friend auto operator!=(const Table&, const Table&) noexcept -> bool;
    friend auto operator<<(std::ostream&, const Table&) -> std::ostream&;

    friend struct std::hash<Table>;

    // TODO maybe return proxy "entry" type
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
     * Returns the location of the call.
     */
    [[nodiscard]] auto call_location() const -> std::optional<Range>;

    /**
     * Returns a reference to the global environment.
     * You can't access local variables with this.
     */
    [[nodiscard]] auto environment() const -> Environment&;

    /**
     * Returns the value of a global variable accessible from the function.
     *
     * Returns Nil if the variable is not accessible or does not exist.
     */
    [[nodiscard]] auto get(const std::string& name) const -> Value;

    /**
     * Returns the arguments given to this function.
     */
    [[nodiscard]] auto arguments() const -> const Vallist&;

    /**
     * Convenience methods for writing functions one or two numeric arguments
     * that should track the origin (e.g. sqrt or pow).
     *
     * Usage:
     *
     * ```cpp
     * auto [arg, origin] = ctx.unary_numeric_arg_helper();
     * origin.reverse = unary_num_reverse(...);
     * // ...
     * ```
     *
     * ```cpp
     * auto [arg1, arg2, origin] = ctx.binary_numeric_arg_helper();
     * origin.reverse = binary_num_reverse(...);
     * // ...
     * ```
     */
    [[nodiscard]] auto unary_numeric_arg_helper() const -> std::tuple<double, UnaryOrigin>;
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

    // friend auto operator<<(std::ostream&, const CallResult&) -> std::ostream&;
};

auto operator==(const CallResult&, const CallResult&) -> bool;
auto operator<<(std::ostream&, const CallResult&) -> std::ostream&;

/**
 * A function (in lua or implemented natively).
 *
 * Notes for implementing native functions:
 *
 * You get a `CallContext` as only argument. You can retrieve the actual
 * arguments and the global environment from the context.
 *
 * When using lua `Value`s you can use the normal c++ operators. Note that `^`
 * performs the pow operation like in lua (and not xor like usually in c++).
 * These operators will track the origin and can be later forces to a different
 * value.
 *
 * Care must be taken when there are control flow constructs in your native
 * function (e.g. `if`, `while`, ...). Because this might break the source change
 * mechanism if you use operators on Values. If you use control flow constructs
 * you have to remove the origin from the returned Value with `Value::remove_origin`.
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

struct NoOrigin {};
auto operator==(const NoOrigin&, const NoOrigin&) noexcept -> bool;
auto operator!=(const NoOrigin&, const NoOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const NoOrigin&) -> std::ostream&;

struct ExternalOrigin {};
auto operator==(const ExternalOrigin&, const ExternalOrigin&) noexcept -> bool;
auto operator!=(const ExternalOrigin&, const ExternalOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const ExternalOrigin&) -> std::ostream&;

// Value was created from a literal in code.
struct LiteralOrigin {
    Range location;
};

auto operator==(const LiteralOrigin&, const LiteralOrigin&) noexcept -> bool;
auto operator!=(const LiteralOrigin&, const LiteralOrigin&) noexcept -> bool;
auto operator<<(std::ostream&, const LiteralOrigin&) -> std::ostream&;

// Value was created in a binary operation (or some functions with two arguments) using lhs and rhs.
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

// Value was created in a unary operation (or some functions with one argument) using val.
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
};

auto operator==(const Origin&, const Origin&) noexcept -> bool;
auto operator!=(const Origin&, const Origin&) noexcept -> bool;
auto operator<<(std::ostream&, const Origin&) -> std::ostream&;

} // namespace minilua

namespace std {

// behaves like std::get(std::variant) but only accepts types as template parameter
template <typename T> auto get(minilua::Origin& origin) -> T& { return std::get<T>(origin.raw()); }

template <typename T> auto get(const minilua::Origin& origin) -> const T& {
    return std::get<T>(origin.raw());
}

} // namespace std

namespace minilua {

/**
 * Represents a value in lua.
 *
 * You can use most normal c++ operators on these value (+, -, *, /, []). If the
 * operation can't be performed on the actual value type an exception will be thrown.
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
     * NOTE: Functions with a parameter of CallContext& does not work.
     */
    template <typename Fn, typename = std::enable_if_t<std::is_invocable_v<Fn, CallContext>>>
    Value(Fn val) : Value(Function(std::forward<Fn>(val))) {}

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
     * Returns the value as a literal string that can be directly inserted in lua code.
     *
     * Throws a 'std::runtime_error' if the value is a function.
     */
    [[nodiscard]] auto to_literal() const -> std::string;

    [[nodiscard]] auto is_valid_identifier() const -> bool;

    [[nodiscard]] auto is_nil() const -> bool;
    [[nodiscard]] auto is_bool() const -> bool;
    [[nodiscard]] auto is_number() const -> bool;
    [[nodiscard]] auto is_string() const -> bool;
    [[nodiscard]] auto is_table() const -> bool;
    [[nodiscard]] auto is_function() const -> bool;

    [[nodiscard]] auto has_origin() const -> bool;

    [[nodiscard]] auto origin() const -> const Origin&;

    [[nodiscard]] auto remove_origin() const -> Value;
    [[nodiscard]] auto with_origin(Origin new_origin) const -> Value;
    auto type() const -> std::string;

    /**
     * Forces this value to become 'new_value'. Does not actually change the
     * value. This will only return a SourceChange that (when applied) would
     * result in the this value being changed.
     *
     * The return value should be returned in NativeFunctions otherwise this
     * does not have an effect.
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

    explicit operator bool() const;

    /*
     * Source location tracking versions of the c++ operators.
     *
     * Can be used in the interpreter to add the source location of the operation.
     */

    /**
     * unary - operator
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
     * unary not operator
     */
    [[nodiscard]] auto invert(std::optional<Range> location = std::nullopt) const -> Value;
    /**
     * unary # operator
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

// behaves like std::get(std::variant) but only accepts types as template parameter
template <typename T> auto get(minilua::Value& value) -> T& { return std::get<T>(value.raw()); }

template <typename T> auto get(const minilua::Value& value) -> const T& {
    return std::get<T>(value.raw());
}

} // namespace std

namespace minilua {

/**
 * Helper functions for writing functions that should be forcable.
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

// use by constructing with lambdas directly and immediately invoke with the CallContext
template <typename Fn, typename Reverse> struct UnaryNumericFunctionHelper {
    Fn function;     // NOLINT
    Reverse reverse; // NOLINT

    auto operator()(const CallContext& ctx) -> Value {
        auto [arg1, origin] = ctx.unary_numeric_arg_helper();
        origin.reverse = unary_num_reverse(this->reverse);
        return Value(this->function(arg1)).with_origin(origin);
    }
};
// deduction guide
template <class... Ts> UnaryNumericFunctionHelper(Ts...) -> UnaryNumericFunctionHelper<Ts...>;

template <typename Fn, typename ReverseLeft, typename ReverseRight>
struct BinaryNumericFunctionHelper {
    Fn function;                // NOLINT
    ReverseLeft reverse_left;   // NOLINT
    ReverseRight reverse_right; // NOLINT

    auto operator()(const CallContext& ctx) -> Value {
        auto [arg1, arg2, origin] = ctx.binary_numeric_args_helper();
        origin.reverse = binary_num_reverse(this->reverse_left, this->reverse_right);
        return Value(this->function(arg1, arg2)).with_origin(origin);
    }
};
// deduction guide
template <class... Ts> BinaryNumericFunctionHelper(Ts...) -> BinaryNumericFunctionHelper<Ts...>;

// helper functions

/**
 * Parse a string into a lua value number.
 */
auto parse_number_literal(const std::string& str) -> Value;
auto parse_string_literal(const std::string& str) -> Value;

} // namespace minilua

#endif
