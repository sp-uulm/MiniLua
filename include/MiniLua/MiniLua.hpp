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

/**
 * This header defines the public api of the MiniLua library.
 *
 * We use the PImpl technique to hide implementation details
 * (https://en.cppreference.com/w/cpp/language/pimpl).
 * This has two benefits:
 *
 * 1. If we change some implementation details the user of the library does not need to recompile
 * 2. We can hide implementation details from the user of the library (e.g. Tree-Sitter)
 */

// TODO do we need integrated performance statistics?
// - it's easy to just meassure how long parse/apply_source_changes and run takes yourself
namespace minilua {

/**
 * This is basically a std::unique_ptr that also support copying.
 *
 * If this type is copied it will make a new heap allocation and copy the value.
 */
template <typename T> class owning_ptr : public std::unique_ptr<T> {
public:
    using std::unique_ptr<T>::unique_ptr;

    owning_ptr() { this->reset(new T()); }

    owning_ptr(const owning_ptr<T>& other) { this->reset(new T(*other.get())); }

    owning_ptr<T>& operator=(const owning_ptr<T>& other) {
        this->reset(new T(*other.get()));
        return *this;
    }
};

template <typename T, typename... Args> owning_ptr<T> make_owning(Args... args) {
    return owning_ptr<T>(new T(std::forward<Args>(args)...));
}

// forward declaration
class Vallist;
class Value;
class Environment;

class Range {};

class CallContext {
    Range location;
    Environment& env;

public:
    CallContext(Environment& env) : env(env) {}

    [[nodiscard]] Range call_location() const;

    /**
     * Returns a reference to the global environment.
     */
    [[nodiscard]] Environment& environment() const;

    /**
     * Returns the value of a variable accessible from the function.
     */
    [[nodiscard]] Value& get(std::string name) const;
};

class SourceChange;
class CallResult {
public:
    CallResult();
    CallResult(Vallist);
    CallResult(SourceChange);
    CallResult(Vallist, SourceChange);
};

struct Nil {};
constexpr bool operator==(Nil, Nil) noexcept;
constexpr bool operator!=(Nil, Nil) noexcept;
std::ostream& operator<<(std::ostream&, Nil);

struct Bool {
    bool value;

    constexpr Bool(bool);
};
constexpr bool operator==(Bool, Bool) noexcept;
constexpr bool operator!=(Bool, Bool) noexcept;
std::ostream& operator<<(std::ostream&, Bool);

struct Number {
    double value;

    constexpr Number(int value) : value(value) {}
    constexpr Number(double);
};
constexpr bool operator==(Number, Number) noexcept;
constexpr bool operator!=(Number, Number) noexcept;
constexpr bool operator<(Number, Number) noexcept;
constexpr bool operator>(Number, Number) noexcept;
constexpr bool operator<=(Number, Number) noexcept;
constexpr bool operator>=(Number, Number) noexcept;
std::ostream& operator<<(std::ostream&, Number);

struct String {
    std::string value;

    String(std::string value);
};
bool operator==(const String& a, const String& b) noexcept;
bool operator!=(const String& a, const String& b) noexcept;
std::ostream& operator<<(std::ostream&, const String&);

class Table {
    struct Impl;
    owning_ptr<Impl> impl;

public:
    Table();
    Table(std::unordered_map<Value, Value>);
    Table(std::initializer_list<std::pair<const Value, Value>> values);

    Table(const Table& other);
    Table(Table&& other) noexcept;
    ~Table() noexcept;
    Table& operator=(const Table& other);
    Table& operator=(Table&& other);
    friend void swap(Table& self, Table& other);

    friend bool operator==(const Table&, const Table&) noexcept;
    friend bool operator!=(const Table&, const Table&) noexcept;
    friend std::ostream& operator<<(std::ostream&, const Table&);
};

} // namespace minilua

namespace std {

template <> struct hash<minilua::Value> {
    size_t operator()(const minilua::Value& value) const { return 0; }
};

} // namespace std

namespace minilua {

class NativeFunction {
    using FnType = CallResult(CallContext);
    std::function<FnType> func;

public:
    template <typename Fn, typename = std::enable_if_t<std::is_invocable_v<Fn, CallContext>>>
    NativeFunction(Fn fn) {
        if constexpr (std::is_convertible_v<Fn, std::function<FnType>>) {
            std::cout << "std::function -> Vallist\n";
            this->func = fn;
        } else if constexpr (std::is_convertible_v<std::invoke_result_t<Fn, CallContext>, Value>) {
            // easy use of functions that return a type that is convertible to Value (e.g. string)
            std::cout << "std::function -> into Value\n";
            this->func = [fn](CallContext ctx) -> CallResult { return CallResult({fn(ctx)}); };
        } else if constexpr (std::is_void_v<std::invoke_result_t<Fn, CallContext>>) {
            std::cout << "lambda wrapper -> void\n";
            // support void functions by returning an empty Vallist
            this->func = [fn](CallContext ctx) -> CallResult {
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
};

std::ostream& operator<<(std::ostream&, const NativeFunction&);

// TODO LuaFunction
// could maybe share a type with NativeFunction (not sure)

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

    Value(const Value&);
    Value(Value&&) noexcept;
    Value& operator=(const Value& other);
    Value& operator=(Value&& other);
    friend void swap(Value& self, Value& other);

    template <typename Fn, typename = std::enable_if_t<std::is_invocable_v<Fn, CallContext>>>
    Value(Fn val) : Value(NativeFunction(std::forward<Fn>(val))) {}

    ~Value();

    Type& get();
    const Type& get() const;
};

bool operator==(const Value&, const Value&) noexcept;
bool operator!=(const Value&, const Value&) noexcept;
std::ostream& operator<<(std::ostream&, const Value&);

class Vallist {
public:
    Vallist();
    Vallist(std::vector<Value>);
    Vallist(std::initializer_list<Value>);

    // template <typename... T>
    // Vallist(T... val) : Vallist(std::vector{Value(std::forward<T>(val))...}) {}
};

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

    void add(std::string name, Value value);

    void add_all(std::unordered_map<std::string, Value> values);
    void add_all(std::initializer_list<std::pair<const std::string, Value>> values);

    Value& get(const std::string& name);

    friend bool operator==(const Environment&, const Environment&) noexcept;
    friend bool operator!=(const Environment&, const Environment&) noexcept;
    friend std::ostream& operator<<(std::ostream&, const Environment&);
};

class SourceChange {};

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
    [[nodiscard]] Environment& environment();

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
    EvalResult run();
};

} // namespace minilua

#endif
