#include "MiniLua/MiniLua.hpp"
#include <cmath>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <variant>

namespace minilua {

// TODO should be in utility header
// template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// struct Location
auto operator<<(std::ostream& os, const Location& self) -> std::ostream& {
    return os << "Location{ line = " << self.line << ", column = " << self.column
              << ", byte = " << self.byte << " }";
}

// struct Range
auto operator<<(std::ostream& os, const Range& self) -> std::ostream& {
    return os << "Range{ start = " << self.start << ", end = " << self.end << " }";
}

// struct SourceChange
auto operator==(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool {
    return lhs.range == rhs.range && lhs.replacement == rhs.replacement;
}
auto operator!=(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const SourceChange& self) -> std::ostream& {
    return os << "SourceChange{ range = " << self.range << ", replacement = \"" << self.replacement
              << "\" }";
}

// value types
//
// struct Nil
auto operator<<(std::ostream& os, Nil /*unused*/) -> std::ostream& {
    return os << "Nil";
}

// struct Bool

auto operator<<(std::ostream& os, Bool self) -> std::ostream& {
    return os << "Bool(" << std::boolalpha << self.value << std::noboolalpha << ")";
}

// struct Number
auto operator<<(std::ostream& os, Number self) -> std::ostream& {
    return os << "Number(" << self.value << ")";
}

// struct String
String::String(std::string value) : value(std::move(value)) {}

auto operator==(const String& a, const String& b) noexcept -> bool {
    return a.value == b.value;
}
auto operator!=(const String& a, const String& b) noexcept -> bool {
    return !(a == b);
}
auto operator<<(std::ostream& os, const String& self) -> std::ostream& {
    return os << "String(\"" << self.value << "\")";
}

// struct Table
struct Table::Impl {
    std::unordered_map<Value, Value> value;
};
Table::Table() : impl(std::make_shared<Impl>()){};
Table::Table(std::unordered_map<Value, Value> value)
    : impl(std::make_shared<Impl>(Impl{.value = std::move(value)})) {}
Table::Table(std::initializer_list<std::pair<const Value, Value>> values) : Table() {
    for (const auto& [key, value] : values) {
        this->impl->value.insert_or_assign(key, value);
    }
}

Table::Table(const Table& other) = default;
Table::Table(Table&& other) noexcept = default;
Table::~Table() noexcept = default;
auto Table::operator=(const Table& other) -> Table& = default;
auto Table::operator=(Table&& other) noexcept -> Table& = default;
void swap(Table& self, Table& other) {
    std::swap(self.impl, other.impl);
}

auto Table::get(const Value& key) -> Value {
    return impl->value.at(key);
}
void Table::set(const Value& key, Value value) {
    impl->value[key] = std::move(value);
}
void Table::set(Value&& key, Value value) {
    impl->value[key] = std::move(value);
}

auto operator==(const Table& a, const Table& b) noexcept -> bool {
    return a.impl == b.impl;
}
auto operator!=(const Table& a, const Table& b) noexcept -> bool {
    return !(a == b);
}
auto operator<<(std::ostream& os, const Table& self) -> std::ostream& {
    os << "Table { ";
    for (const auto& [key, value] : self.impl->value) {
        os << "[" << key << "] = " << value << ", ";
    }
    return os << " }";
}

// struct NativeFunction
auto operator<<(std::ostream& os, const NativeFunction&) -> std::ostream& {
    return os << "NativeFunction";
}

// class Value
struct Value::Impl {
    Type val;
};

Value::Value() = default;
Value::Value(Value::Type val) : impl(make_owning<Impl>(Impl{.val = std::move(val)})) {}
Value::Value(Nil val) : impl(make_owning<Impl>(Impl{.val = val})) {}
Value::Value(Bool val) : impl(make_owning<Impl>(Impl{.val = val})) {}
Value::Value(bool val) : Value(Bool(val)) {}
Value::Value(Number val) : impl(make_owning<Impl>(Impl{.val = val})) {}
Value::Value(int val) : Value(Number(val)) {}
Value::Value(double val) : Value(Number(val)) {}
Value::Value(String val) : impl(make_owning<Impl>(Impl{.val = val})) {}
Value::Value(std::string val) : Value(String(std::move(val))) {}
Value::Value(const char* val) : Value(String(val)) {}
Value::Value(Table val) : impl(make_owning<Impl>(Impl{.val = val})) {}
Value::Value(NativeFunction val) : impl(make_owning<Impl>(Impl{.val = val})) {}

Value::Value(const Value& other) = default;
Value::Value(Value&& other) noexcept = default;
Value::~Value() = default;
auto Value::operator=(const Value& other) -> Value& = default;
auto Value::operator=(Value&& other) -> Value& = default;
void swap(Value& self, Value& other) {
    std::swap(self.impl, other.impl);
}

auto Value::get() -> Value::Type& {
    return impl->val;
}
auto Value::get() const -> const Value::Type& {
    return impl->val;
}

auto operator==(const Value& a, const Value& b) noexcept -> bool {
    return a.get() == b.get();
}
auto operator!=(const Value& a, const Value& b) noexcept -> bool {
    return !(a == b);
}
auto operator<<(std::ostream& os, const Value& self) -> std::ostream& {
    std::visit([&](const auto& value) { os << "Value(" << value << ")"; }, self.get());
    return os;
}

} // namespace minilua

namespace std {
auto std::hash<minilua::Value>::operator()(const minilua::Value& value) const -> size_t {
    return std::hash<minilua::Value::Type>()(value.get());
}
auto std::hash<minilua::Nil>::operator()(const minilua::Nil& /*value*/) const -> size_t {
    // lua does not allow using nil as a table key
    // but we are not allowed to throw inside of std::hash
    return 0;
}
auto std::hash<minilua::Bool>::operator()(const minilua::Bool& value) const -> size_t {
    return 0;
}
auto std::hash<minilua::Number>::operator()(const minilua::Number& value) const -> size_t {
    // lua does not allow using NaN as a table key
    // but we are not allowed to throw inside of std::hash
    if (std::isnan(value.value)) {
        return 0;
    }
    return std::hash<double>()(value.value);
}
auto std::hash<minilua::String>::operator()(const minilua::String& value) const -> size_t {
    return std::hash<std::string>()(value.value);
}
auto std::hash<minilua::Table>::operator()(const minilua::Table& value) const -> size_t {
    return std::hash<decltype(value.impl)>()(value.impl);
}
auto std::hash<minilua::NativeFunction>::operator()(const minilua::NativeFunction& value) const
    -> size_t {
    return std::hash<decltype(value.func)>()(value.func);
}
} // namespace std

namespace minilua {

struct CallContext::Impl {
    Range location;
    Environment& env;
    Vallist args;
};
CallContext::CallContext(Environment& env) : impl(make_owning<Impl>(Impl{.env = env})) {}
CallContext::CallContext(const CallContext& other) = default;
CallContext::CallContext(CallContext&& other) noexcept = default;
CallContext::~CallContext() = default;

auto CallContext::call_location() const -> Range {
    return impl->location;
}
auto CallContext::environment() const -> Environment& {
    return impl->env;
}
auto CallContext::get(std::string name) const -> Value& {
    return impl->env.get(name);
}
auto CallContext::arguments() const -> const Vallist& {
    return impl->args;
}

auto operator<<(std::ostream& os, const CallContext& self) -> std::ostream& {
    return os << "CallContext{ location = " << self.impl->location
              << ", environment = " << self.impl->env << ", arguments = " << self.impl->args
              << " }";
}

void Environment::add_default_stdlib() {}
void Environment::add(std::string name, Value value) {
    global.insert_or_assign(name, value);
}

void Environment::add_all(std::unordered_map<std::string, Value> values) {
    global.insert(values.begin(), values.end());
}
void Environment::add_all(std::initializer_list<std::pair<const std::string, Value>> values) {
    global.insert(values.begin(), values.end());
}

auto Environment::get(const std::string& name) -> Value& {
    return this->global.at(name);
}

auto operator==(const Environment& a, const Environment& b) noexcept -> bool {
    return a.global == b.global;
}
auto operator!=(const Environment& a, const Environment& b) noexcept -> bool {
    return !(a == b);
}
auto operator<<(std::ostream& os, const Environment& self) -> std::ostream& {
    os << "Environment{";
    for (const auto& [key, value] : self.global) {
        os << "[\"" << key << "\"] = " << value << ", ";
    }
    return os << "}";
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

    auto run() -> EvalResult {
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

auto Interpreter::environment() -> Environment& {
    return impl->env;
}
void Interpreter::parse(std::string source_code) {
    impl->parse(std::move(source_code));
}
void Interpreter::apply_source_changes(std::vector<SourceChange> changes) {
    impl->apply_source_changes(std::move(changes));
}
auto Interpreter::run() -> EvalResult {
    return impl->run();
}

CallResult::CallResult() {
    std::cout << "CallResult\n";
}
CallResult::CallResult(Vallist) {
    std::cout << "CallResult(Vallist)\n";
}
CallResult::CallResult(std::vector<Value> values) : CallResult(Vallist(values)) {}
CallResult::CallResult(std::initializer_list<Value> values) : CallResult(Vallist(values)) {}
CallResult::CallResult(SourceChange) {
    std::cout << "CallResult(SourceChange)\n";
}
CallResult::CallResult(Vallist, SourceChange) {
    std::cout << "CallResult(Vallist, SourceChange)\n";
}

// class Vallist
struct Vallist::Impl {
    std::vector<Value> values;
};
Vallist::Vallist() {
    std::cout << "Vallist()\n";
}
Vallist::Vallist(std::vector<Value>) {
    std::cout << "Vallist(vector)\n";
}
Vallist::Vallist(std::initializer_list<Value>) {
    std::cout << "Vallist(<init-list>)\n";
}

Vallist::Vallist(const Vallist&) = default;
Vallist::Vallist(Vallist&&) noexcept = default;
Vallist::~Vallist() = default;

auto Vallist::size() const -> size_t {
    return impl->values.size();
}
auto Vallist::get(size_t index) const -> const Value& {
    return impl->values.at(index);
}
auto Vallist::begin() const -> std::vector<Value>::const_iterator {
    return impl->values.cbegin();
}
auto Vallist::end() const -> std::vector<Value>::const_iterator {
    return impl->values.cend();
}

auto operator<<(std::ostream& os, const Vallist& self) -> std::ostream& {
    os << "Vallist{ ";
    for (const auto& value : self.impl->values) {
        os << value << ", ";
    }
    os << "}";
    return os;
}

} // namespace minilua
