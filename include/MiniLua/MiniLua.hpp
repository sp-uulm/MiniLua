#ifndef MINILUA_HPP
#define MINILUA_HPP

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "utils.hpp"

/**
 * This header defines the public api of the MiniLua library.
 *
 * We use the PImpl technique to hide implementation details (see below).
 *
 * TODO some more documentation on where to start and design decisions
 *
 * ---
 *
 * PImpl technique:
 *
 * Also see: https://en.cppreference.com/w/cpp/language/pimpl
 *
 * With the PImpl technique we hide all behaviour using a private nested forward
 * declaration of a struct or class. This has two important uses:
 *
 * 1. hiding the implementation details from the user
 * 2. breaking up cycles of incomplete types
 *    (this works because the cyclic reference are now in the cpp instead of the header)
 *
 * For this to work classes using the PImpl technique can't *use* the private nested
 * forward declaration. This means we have to declare but not define the methods
 * in the header. This includes special member functions like copy-constructor and
 * destructor. Then these methods have to be implemented in the cpp file and the
 * only difference is basically instead of using `this->` they have to use
 * `this->impl->` or just `impl->`.
 *
 * A class with the PImpl technique usually looks like this:
 *
 * ```
 * // in the header
 * class Something {
 *   struct Impl;
 *   owning_ptr<Impl> impl; // any pointer type is ok here
 *
 * public:
 *   // normal constructor declarations
 *   Something(const Something&);
 *   Something(Something&&) noexcept;
 *   Something& operator=(const Something& other);
 *   Something& operator=(Something&& other);
 *   friend void swap(Something& self, Something& other);
 * };
 *
 * // in the cpp
 * struct Something::Impl {
 *   // fields you would have put in class Something
 * };
 * Something::Something(const Something&) = default;
 * Something(Something&&) noexcept = default;
 * Something& operator=(const Something& other) = default;
 * Something& operator=(Something&& other) = default;
 * void swap(Something& self, Something& other) {
 *   std::swap(self.impl, other.impl);
 * }
 * ```
 *
 * If the nested `struct Impl` is not default constructible you have to provide
 * the implementation of the copy-/move-constructors and -operators manually and
 * can't use `= default`.
 *
 * `owning_ptr<T>` was chosen for the pointer type because it behaves like a
 * normal `T` (but lives on the heap). It's move, copy and lifetime semantics
 * are identical to the one of `T`.
 *
 * Is is also possible to choose another pointer type like `std::unique_ptr` or
 * `std::shared_ptr`. You should probably avoid using raw pointer `T*` because
 * lifetime management is harder with raw pointers.
 */
namespace minilua {

/**
 * Represents a location in source code.
 *
 * NOTE: The comparison operators only consider the byte field. If you want
 * correct results you should only compare locations that were generated from
 * the same source code.
 */
struct Location {
    uint32_t line;
    uint32_t column;
    uint32_t byte;
};

constexpr auto operator==(Location lhs, Location rhs) noexcept -> bool {
    return lhs.byte == rhs.byte;
}
constexpr auto operator!=(Location lhs, Location rhs) noexcept -> bool {
    return !(lhs == rhs);
}
constexpr auto operator<(Location lhs, Location rhs) noexcept -> bool {
    return lhs.byte < rhs.byte;
}
constexpr auto operator<=(Location lhs, Location rhs) noexcept -> bool {
    return lhs.byte <= rhs.byte;
}
constexpr auto operator>(Location lhs, Location rhs) noexcept -> bool {
    return lhs.byte > rhs.byte;
}
constexpr auto operator>=(Location lhs, Location rhs) noexcept -> bool {
    return lhs.byte >= rhs.byte;
}
auto operator<<(std::ostream&, const Location&) -> std::ostream&;

struct Range {
    Location start;
    Location end;
};

constexpr auto operator==(Range lhs, Range rhs) noexcept -> bool {
    return lhs.start == rhs.start && lhs.end == rhs.end;
}
constexpr auto operator!=(Range lhs, Range rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream&, const Range&) -> std::ostream&;

struct SourceChange {
    Range range;
    std::string replacement;
};

auto operator==(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool;
auto operator!=(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SourceChange&) -> std::ostream&;

// forward declaration
class Value;
class Environment;

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
    Vallist(Vallist&&) noexcept;
    ~Vallist();

    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto get(size_t index) const -> const Value&;
    [[nodiscard]] auto begin() const -> std::vector<Value>::const_iterator;
    [[nodiscard]] auto end() const -> std::vector<Value>::const_iterator;

    friend auto operator<<(std::ostream&, const Vallist&) -> std::ostream&;
};

class CallContext {
    struct Impl;
    owning_ptr<Impl> impl;

public:
    CallContext(Environment& env);
    CallContext(const CallContext&);
    CallContext(CallContext&&) noexcept;
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

struct Nil {};
constexpr auto operator==(Nil, Nil) noexcept -> bool {
    return true;
}
constexpr auto operator!=(Nil, Nil) noexcept -> bool {
    return false;
}
auto operator<<(std::ostream&, Nil) -> std::ostream&;

struct Bool {
    bool value;

    constexpr Bool(bool value) : value(value) {}
};
constexpr auto operator==(Bool lhs, Bool rhs) noexcept -> bool {
    return lhs.value == rhs.value;
}
constexpr auto operator!=(Bool lhs, Bool rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream&, Bool) -> std::ostream&;

struct Number {
    double value;

    constexpr Number(int value) : value(value) {}
    constexpr Number(double value) : value(value) {}
};
constexpr auto operator==(Number lhs, Number rhs) noexcept -> bool {
    return lhs.value == rhs.value;
}
constexpr auto operator!=(Number lhs, Number rhs) noexcept -> bool {
    return !(lhs == rhs);
}
constexpr auto operator<(Number lhs, Number rhs) noexcept -> bool {
    return lhs.value < rhs.value;
}
constexpr auto operator>(Number lhs, Number rhs) noexcept -> bool {
    return lhs.value > rhs.value;
}
constexpr auto operator<=(Number lhs, Number rhs) noexcept -> bool {
    return lhs.value <= rhs.value;
}
constexpr auto operator>=(Number lhs, Number rhs) noexcept -> bool {
    return lhs.value >= rhs.value;
}
auto operator<<(std::ostream&, Number) -> std::ostream&;

struct String {
    const std::string value;

    String(std::string value);
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
            *this->func = [fn](CallContext ctx) -> CallResult {
                return CallResult({fn(ctx)});
            };
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

class Value {
    struct Impl;
    owning_ptr<Impl> impl;
    // Type val;

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
    Value(Value&&) noexcept;
    auto operator=(const Value& other) -> Value&;
    auto operator=(Value&& other) noexcept -> Value&;
    friend void swap(Value& self, Value& other);

    ~Value();

    auto get() -> Type&;
    auto get() const -> const Type&;
};

auto operator==(const Value&, const Value&) noexcept -> bool;
auto operator!=(const Value&, const Value&) noexcept -> bool;
auto operator<<(std::ostream&, const Value&) -> std::ostream&;

/**
 * Represents the global environment/configuration for the 'Interpreter'.
 *
 * This contains things like global variables (including functions), etc.
 */
class Environment {
    // holds global variabes (including functions)
    std::unordered_map<std::string, Value> global;

public:
    // equivalent to the old env->populate_stdlib()
    void add_default_stdlib();

    void add(const std::string& name, Value value);
    void add(std::string&& name, Value value);

    void add_all(std::unordered_map<std::string, Value> values);
    void add_all(std::initializer_list<std::pair<const std::string, Value>> values);

    auto get(const std::string& name) -> Value&;

    auto size() const -> size_t;

    friend auto operator==(const Environment&, const Environment&) noexcept -> bool;
    friend auto operator!=(const Environment&, const Environment&) noexcept -> bool;
    friend auto operator<<(std::ostream&, const Environment&) -> std::ostream&;
};

struct SuggestedSourceChange {
    // can be filled in by the function creating the suggestion
    std::optional<std::string> origin;
    // hint for the source locations that would be modified
    // TODO maybe should be part of the SourceChange
    std::string hint;
    // TODO maybe this needs to be a vector
    SourceChange change;
};

struct EvalResult {
    std::vector<SuggestedSourceChange> source_change_suggestions;
};

class Interpreter {
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    Interpreter();
    Interpreter(std::string initial_source_code);
    ~Interpreter();

    /**
     * Returns the environment for modification.
     */
    [[nodiscard]] auto environment() -> Environment&;

    /**
     * Parse fresh source code.
     */
    void parse(std::string source_code);

    /**
     * Applies source changes.
     *
     * These can be created inside or outside the interpreter.
     */
    void apply_source_changes(std::vector<SourceChange>);

    /**
     * Run the parsed program.
     *
     * TODO should the user be able to return a result from lua?
     * - we would need to make the actual syntax tree part of the public interface
     * - OR we need to serialize it in some way (e.g. json)
     *
     * TODO should the user be able to provide parameters?
     * - not sure how you would provide them to lua
     */
    auto run() -> EvalResult;
};

} // namespace minilua

#endif
