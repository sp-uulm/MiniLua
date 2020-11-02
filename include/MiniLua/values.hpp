#ifndef MINILUA_VALUES_HPP
#define MINILUA_VALUES_HPP

#include <functional>
#include <string>
#include <unordered_map>
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
    auto operator=(Vallist &&) -> Vallist&;
    ~Vallist();

    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto get(size_t index) const -> const Value&;
    [[nodiscard]] auto begin() const -> std::vector<Value>::const_iterator;
    [[nodiscard]] auto end() const -> std::vector<Value>::const_iterator;

    friend auto operator<<(std::ostream&, const Vallist&) -> std::ostream&;
};

struct Nil {};
constexpr auto operator==(Nil, Nil) noexcept -> bool { return true; }
constexpr auto operator!=(Nil, Nil) noexcept -> bool { return false; }
auto operator<<(std::ostream&, Nil) -> std::ostream&;

struct Bool {
    bool value;

    constexpr Bool(bool value) : value(value) {}
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

    constexpr Number(int value) : value(value) {}
    constexpr Number(double value) : value(value) {}
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

struct String {
    std::string value;

    String(std::string value);

    friend void swap(String& self, String& other);
};
auto operator==(const String& a, const String& b) noexcept -> bool;
auto operator!=(const String& a, const String& b) noexcept -> bool;
auto operator<<(std::ostream&, const String&) -> std::ostream&;

class Table {
    struct Impl;
    std::shared_ptr<Impl> impl;

public:
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

    friend auto operator==(const Table&, const Table&) noexcept -> bool;
    friend auto operator!=(const Table&, const Table&) noexcept -> bool;
    friend auto operator<<(std::ostream&, const Table&) -> std::ostream&;

    friend struct std::hash<Table>;

    // TODO maybe return proxy "entry" type
    auto operator[](const Value&) -> Value&;
    auto operator[](const Value&) const -> const Value&;
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
    auto operator=(CallContext &&) -> CallContext&;
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

    /**
     * Forces the 'target' value to become 'new_value' which will trigger
     * create source change that is returned by 'Interpreter::evaluate'.
     *
     * The return value should be returned in NativeFunctions otherwise this
     * does not have an effect.
     *
     * This throws an exception if the types of the values didn't match.
     */
    auto force_value(Value target, Value new_value) -> SuggestedSourceChange;

    friend auto operator<<(std::ostream&, const CallContext&) -> std::ostream&;
};

class CallResult {
public:
    CallResult();
    CallResult(Vallist);
    CallResult(std::vector<Value>);
    CallResult(std::initializer_list<Value>);
    CallResult(SuggestedSourceChange);
    CallResult(Vallist, SuggestedSourceChange);

    // friend auto operator<<(std::ostream&, const CallResult&) -> std::ostream&;
};

class NativeFunction {
    using FnType = CallResult(CallContext);
    std::shared_ptr<std::function<FnType>> func;

public:
    template <typename Fn, typename = std::enable_if_t<std::is_invocable_v<Fn, CallContext>>>
    NativeFunction(Fn fn) {
        this->func = std::make_shared<std::function<FnType>>();

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

    friend void swap(NativeFunction& self, NativeFunction& other);

    friend struct std::hash<NativeFunction>;
};

auto operator<<(std::ostream&, const NativeFunction&) -> std::ostream&;

// TODO LuaFunction
// could maybe share a type with NativeFunction (not sure)

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
// TODO should we allow this?
template <> struct hash<minilua::NativeFunction> {
    auto operator()(const minilua::NativeFunction& value) const -> size_t;
};

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
    using Type = std::variant<Nil, Bool, Number, String, Table, NativeFunction>;

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
    Value(NativeFunction val);

    /**
     * NOTE: Functions with a parameter of CallContext& does not work.
     */
    template <typename Fn, typename = std::enable_if_t<std::is_invocable_v<Fn, CallContext>>>
    Value(Fn val) : Value(NativeFunction(std::forward<Fn>(val))) {}

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

    auto get() -> Type&;
    [[nodiscard]] auto get() const -> const Type&;

    auto operator[](const Value&) -> Value&;
    auto operator[](const Value&) const -> const Value&;
};

auto operator==(const Value&, const Value&) noexcept -> bool;
auto operator!=(const Value&, const Value&) noexcept -> bool;
auto operator<<(std::ostream&, const Value&) -> std::ostream&;

auto operator+(const Value&, const Value&) -> Value;
auto operator-(const Value&, const Value&) -> Value;
auto operator*(const Value&, const Value&) -> Value;
auto operator/(const Value&, const Value&) -> Value;
auto operator&(const Value&, const Value&) -> Value;
auto operator|(const Value&, const Value&) -> Value;
auto operator^(const Value&, const Value&) -> Value;
auto operator&&(const Value&, const Value&) -> Value;
auto operator||(const Value&, const Value&) -> Value;

} // namespace minilua

#endif
