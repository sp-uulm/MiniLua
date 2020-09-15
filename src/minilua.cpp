#include "MiniLua/MiniLua.hpp"
#include <ios>
#include <iostream>
#include <utility>
#include <variant>

namespace minilua {

// TODO should be in utility header
// template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// value types
//
// struct Nil
constexpr bool operator==(Nil, Nil) noexcept { return true; }
constexpr bool operator!=(Nil, Nil) noexcept { return false; }
std::ostream& operator<<(std::ostream& o, Nil) { return o << "nil"; }

// struct Bool
constexpr Bool::Bool(bool value) : value(value) {}

constexpr bool operator==(Bool a, Bool b) noexcept { return a.value == b.value; }
constexpr bool operator!=(Bool a, Bool b) noexcept { return !(a == b); }
std::ostream& operator<<(std::ostream& o, Bool self) {
    return o << std::boolalpha << self.value << std::noboolalpha;
}

// struct Number
// constexpr Number::Number(int value) : value(value) {}
constexpr Number::Number(double value) : value(value) {}

constexpr bool operator==(Number a, Number b) noexcept { return a.value == b.value; }
constexpr bool operator!=(Number a, Number b) noexcept { return !(a == b); }
constexpr bool operator<(Number a, Number b) noexcept { return a.value < b.value; }
constexpr bool operator>(Number a, Number b) noexcept { return a.value > b.value; }
constexpr bool operator<=(Number a, Number b) noexcept { return a.value <= b.value; }
constexpr bool operator>=(Number a, Number b) noexcept { return a.value >= b.value; }
std::ostream& operator<<(std::ostream& o, Number self) { return o << self.value; }

// struct String
String::String(std::string value) : value(value) {}

bool operator==(const String& a, const String& b) noexcept { return a.value == b.value; }
bool operator!=(const String& a, const String& b) noexcept { return !(a == b); }
std::ostream& operator<<(std::ostream& o, const String& self) {
    return o << "\"" << self.value << "\"";
}

// struct Table
struct Table::Impl {
    std::unordered_map<Value, Value> value;
};
Table::Table() = default;
Table::Table(std::unordered_map<Value, Value> value)
    : impl(make_owning<Impl>(Impl{.value = std::move(value)})) {}
Table::Table(std::initializer_list<std::pair<const Value, Value>> values) {
    for (const auto& [key, value] : values) {
        this->impl->value.insert_or_assign(key, value);
        std::cout << "\t" << key << " = " << value << "\n";
    }
    std::cout << "new Table: " << *this << "\n";
}

Table::Table(const Table& other) : impl(other.impl) {}
Table::Table(Table&& other) noexcept : impl(other.impl) {}
Table::~Table() noexcept = default;
Table& Table::operator=(const Table& other) {
    impl = other.impl;
    return *this;
}
Table& Table::operator=(Table&& other) {
    swap(*this, other);
    return *this;
}
void swap(Table& self, Table& other) { std::swap(self.impl, other.impl); }

bool operator==(const Table& a, const Table& b) noexcept { return a.impl->value == b.impl->value; }
bool operator!=(const Table& a, const Table& b) noexcept { return !(a == b); }
std::ostream& operator<<(std::ostream& o, const Table& self) {
    o << "{";

    for (const auto& [key, value] : self.impl->value) {
        o << "[" << key << "] = " << value << ", ";
    }

    return o << "}";
}

// struct NativeFunction
std::ostream& operator<<(std::ostream& o, const NativeFunction&) { return o << "<NativeFunction>"; }

// class Value
struct Value::Impl {
    Type val;
};

Value::Value() = default;
Value::Value(Value::Type val) : impl(make_owning<Impl>(Impl{.val = std::move(val)})) {
    std::cout << "Value(std::variant)\n";
}
Value::Value(Nil val) : impl(make_owning<Impl>(Impl{.val = val})) { std::cout << "Value(Nil)\n"; }
Value::Value(Bool val) : impl(make_owning<Impl>(Impl{.val = val})) {
    std::cout << "Value(Bool: " << val << ")\n";
}
Value::Value(bool val) : Value(Bool(val)) { std::cout << "Value(bool)\n"; }
Value::Value(Number val) : impl(make_owning<Impl>(Impl{.val = val})) {
    std::cout << "Value(Number: " << val << ")\n";
}
Value::Value(int val) : Value(Number(val)) { std::cout << "Value(int)\n"; }
Value::Value(double val) : Value(Number(val)) { std::cout << "Value(double)\n"; }
Value::Value(String val) : impl(make_owning<Impl>(Impl{.val = val})) {
    std::cout << "Value(String)\n";
}
Value::Value(std::string val) : Value(String(std::move(val))) {
    std::cout << "Value(std::string)\n";
}
Value::Value(const char* val) : Value(String(val)) { std::cout << "Value(const char*)\n"; }
Value::Value(Table val) : impl(make_owning<Impl>(Impl{.val = val})) {
    std::cout << "Value(Table)\n";
}
Value::Value(NativeFunction val) : impl(make_owning<Impl>(Impl{.val = val})) {
    std::cout << "Value(NativeFunction)\n";
}

Value::Value(const Value& other) = default;
Value::Value(Value&& other) noexcept = default;
Value::~Value() = default;
Value& Value::operator=(const Value& other) = default;
Value& Value::operator=(Value&& other) = default;
void swap(Value& self, Value& other) { std::swap(self.impl, other.impl); }

Value::Type& Value::get() { return impl->val; }
const Value::Type& Value::get() const { return impl->val; }

bool operator==(const Value& a, const Value& b) noexcept { return a.get() == b.get(); }
bool operator!=(const Value& a, const Value& b) noexcept { return !(a == b); }
std::ostream& operator<<(std::ostream& o, const Value& self) {
    std::visit([&](const auto& value) { o << value; }, self.get());

    return o;
}

struct CallContext::Impl {
    Range location;
    Environment& env;
    Vallist args;
};
CallContext::CallContext(Environment& env) : impl(make_owning<Impl>(Impl{.env = env})) {}
CallContext::CallContext(const CallContext& other) = default;
CallContext::CallContext(CallContext&& other) noexcept = default;
CallContext::~CallContext() = default;

Range CallContext::call_location() const { return impl->location; }
Environment& CallContext::environment() const { return impl->env; }
Value& CallContext::get(std::string name) const { return impl->env.get(name); }
const Vallist& CallContext::arguments() const { return impl->args; }

void Environment::add_default_stdlib() {}
void Environment::add(std::string name, Value value) { global.insert_or_assign(name, value); }

void Environment::add_all(std::unordered_map<std::string, Value> values) {
    global.insert(values.begin(), values.end());
}
void Environment::add_all(std::initializer_list<std::pair<const std::string, Value>> values) {
    global.insert(values.begin(), values.end());
}

Value& Environment::get(const std::string& name) { return this->global.at(name); }

bool operator==(const Environment& a, const Environment& b) noexcept {
    return a.global == b.global;
}
bool operator!=(const Environment& a, const Environment& b) noexcept { return !(a == b); }
std::ostream& operator<<(std::ostream& o, const Environment& self) {
    o << "Environment {";

    for (const auto& [key, value] : self.global) {
        o << "[\"" << key << "\"] = " << value << ", ";
    }

    return o << "}";
}

class Parser {
    std::string source;

public:
    Parser(std::string source) : source(std::move(source)) {}
};

class Tree {};

struct Interpreter::Impl {
    Parser parser;
    Tree tree;
    Environment env;

    Impl(Parser parser, Environment env) : parser(parser), env(std::move(env)) {}

    void parse(std::string source) {
        // TODO
        std::cout << "parse\n";
    }

    void apply_source_changes(std::vector<SourceChange> changes) {
        // TODO
        std::cout << "apply_source_changes\n";
    }

    EvalResult run() {
        std::cout << "run\n";
        // TODO
        return EvalResult();
    }
};

Interpreter::Interpreter() : Interpreter("") {}
Interpreter::Interpreter(std::string initial_source_code)
    : impl(std::make_unique<Interpreter::Impl>(
          Parser(std::move(initial_source_code)), Environment())) {
    // TODO initialize parser with source code
}
Interpreter::~Interpreter() = default;

Environment& Interpreter::environment() { return impl->env; }
void Interpreter::parse(std::string source_code) { impl->parse(std::move(source_code)); }
void Interpreter::apply_source_changes(std::vector<SourceChange> changes) {
    impl->apply_source_changes(std::move(changes));
}
EvalResult Interpreter::run() { return impl->run(); }

CallResult::CallResult() { std::cout << "CallResult\n"; }
CallResult::CallResult(Vallist) { std::cout << "CallResult(Vallist)\n"; }
CallResult::CallResult(std::vector<Value> values) : CallResult(Vallist(values)) {}
CallResult::CallResult(std::initializer_list<Value> values) : CallResult(Vallist(values)) {}
CallResult::CallResult(SourceChange) { std::cout << "CallResult(SourceChange)\n"; }
CallResult::CallResult(Vallist, SourceChange) {
    std::cout << "CallResult(Vallist, SourceChange)\n";
}

// class Vallist
struct Vallist::Impl {
    std::vector<Value> values;
};
Vallist::Vallist() { std::cout << "Vallist()\n"; }
Vallist::Vallist(std::vector<Value>) { std::cout << "Vallist(vector)\n"; }
Vallist::Vallist(std::initializer_list<Value>) { std::cout << "Vallist(<init-list>)\n"; }

Vallist::Vallist(const Vallist&) = default;
Vallist::Vallist(Vallist&&) noexcept = default;
Vallist::~Vallist() = default;

size_t Vallist::size() const { return impl->values.size(); }
const Value& Vallist::get(size_t index) const { return impl->values.at(index); }
std::vector<Value>::const_iterator Vallist::begin() const { return impl->values.cbegin(); }
std::vector<Value>::const_iterator Vallist::end() const { return impl->values.cend(); }

} // namespace minilua
