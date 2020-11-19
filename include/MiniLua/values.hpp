#ifndef MINILUA_VALUES_HPP
#define MINILUA_VALUES_HPP

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "environment.hpp"
#include "source_change.hpp"
#include "utils.hpp"

#define DELEGATE_OP(TYPE, OP)                                                                      \
    constexpr auto operator OP(const TYPE& lhs, const TYPE& rhs)->TYPE {                           \
        return TYPE(lhs.value OP rhs.value);                                                       \
    }

namespace minilua {

template <size_t, class T> using T_ = T;

// forward declaration
class Value;

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

    [[nodiscard]] auto size() const -> size_t;

    /**
     * Returns the value at the given index.
     *
     * If the value does not exist a Nil value will be returned.
     */
    [[nodiscard]] auto get(size_t index) const -> const Value&;

    /**
     * Iterator over the Vallist. Can be used in e.g. for loops.
     */
    [[nodiscard]] auto begin() const -> std::vector<Value>::const_iterator;
    [[nodiscard]] auto end() const -> std::vector<Value>::const_iterator;

    // helper for next method
    template <std::size_t... Is>
    [[nodiscard]] auto tuple(std::index_sequence<Is...>) const
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
     */
    template <std::size_t N> [[nodiscard]] auto tuple() const {
        return tuple(std::make_index_sequence<N>{});
    }

    friend auto operator<<(std::ostream&, const Vallist&) -> std::ostream&;
};

struct Nil {
    constexpr static const std::string_view TYPE = "nil";

    [[nodiscard]] auto to_literal() const -> std::string;

    explicit operator bool() const;
};
constexpr auto operator==(Nil, Nil) noexcept -> bool { return true; }
constexpr auto operator!=(Nil, Nil) noexcept -> bool { return false; }
auto operator<<(std::ostream&, Nil) -> std::ostream&;

struct Bool {
    bool value;

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
    double value;

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
    std::string value;

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

class Table {
    struct Impl;
    std::shared_ptr<Impl> impl;

public:
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
    void set(const Value& key, Value value);
    void set(Value&& key, Value value);

    [[nodiscard]] auto to_literal() const -> std::string;

    friend auto operator==(const Table&, const Table&) noexcept -> bool;
    friend auto operator!=(const Table&, const Table&) noexcept -> bool;
    friend auto operator<<(std::ostream&, const Table&) -> std::ostream&;

    friend struct std::hash<Table>;

    // TODO maybe return proxy "entry" type
    auto operator[](const Value&) -> Value&;
    auto operator[](const Value&) const -> const Value&;

    explicit operator bool() const;
};

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

    [[nodiscard]] auto call_location() const -> Range;

    /**
     * Returns a reference to the global environment.
     */
    [[nodiscard]] auto environment() const -> Environment&;

    /**
     * Returns the value of a variable accessible from the function.
     */
    [[nodiscard]] auto get(const std::string& name) const -> Value&;

    /**
     * Returns the arguments given to this function.
     */
    [[nodiscard]] auto arguments() const -> const Vallist&;

    friend auto operator<<(std::ostream&, const CallContext&) -> std::ostream&;
};

class CallResult {
public:
    CallResult();
    CallResult(Vallist);
    CallResult(std::vector<Value>);
    CallResult(std::initializer_list<Value>);
    CallResult(SourceChange);
    CallResult(Vallist, SourceChange);

    // friend auto operator<<(std::ostream&, const CallResult&) -> std::ostream&;
};

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

    explicit operator bool() const;

    friend void swap(Function& self, Function& other);

    friend struct std::hash<Function>;
};

auto operator<<(std::ostream&, const Function&) -> std::ostream&;

// TODO LuaFunction
// could maybe share a type with NativeFunction (e.g. by providing lambdas)
//
// Requires:
// - reference to ast of function definition
// - copy of enclosing environment

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

// TODO Origin does not need to be public (except maybe ExternalOrigin)
struct NoOrigin {};
struct ExternalOrigin {};
struct LiteralOrigin {
    Range location;
};
struct BinaryOrigin {
    owning_ptr<Value> lhs;
    owning_ptr<Value> rhs;
    Range location;
};
struct UnaryOrigin {
    owning_ptr<Value> val;
    Range location;
};
struct Origin {
    std::variant<NoOrigin, ExternalOrigin, LiteralOrigin, BinaryOrigin, UnaryOrigin> origin;
};

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

    [[nodiscard]] auto remove_origin() const -> Value;
    [[nodiscard]] auto with_origin(Origin new_origin) const -> Value;

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
    auto force(Value new_value, std::string origin = "") -> SourceChange;

    auto operator[](const Value&) -> Value&;
    auto operator[](const Value&) const -> const Value&;

    explicit operator bool() const;

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

#endif
