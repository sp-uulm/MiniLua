#include "MiniLua/values.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace minilua {

// struct Nil
auto operator<<(std::ostream& os, Nil /*unused*/) -> std::ostream& { return os << "Nil"; }

// struct Bool

auto operator<<(std::ostream& os, Bool self) -> std::ostream& {
    return os << "Bool(" << std::boolalpha << self.value << std::noboolalpha << ")";
}

// struct Number
auto operator<<(std::ostream& os, Number self) -> std::ostream& {
    return os << "Number(" << self.value << ")";
}
auto operator^(Number lhs, Number rhs) -> Number { return std::pow(lhs.value, rhs.value); }
auto operator%(Number lhs, Number rhs) -> Number { return Number(std::fmod(lhs.value, rhs.value)); }

// struct String
String::String(std::string value) : value(std::move(value)) {}

void swap(String& self, String& other) { std::swap(self.value, other.value); }

auto operator==(const String& a, const String& b) noexcept -> bool { return a.value == b.value; }
auto operator!=(const String& a, const String& b) noexcept -> bool { return !(a == b); }
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
void swap(Table& self, Table& other) { std::swap(self.impl, other.impl); }

auto Table::get(const Value& key) -> Value { return impl->value.at(key); }
void Table::set(const Value& key, Value value) { impl->value[key] = std::move(value); }
void Table::set(Value&& key, Value value) { impl->value[key] = std::move(value); }

auto Table::operator[](const Value& index) -> Value& { return impl->value[index]; }
auto Table::operator[](const Value& index) const -> const Value& { return impl->value[index]; }

auto operator==(const Table& a, const Table& b) noexcept -> bool { return a.impl == b.impl; }
auto operator!=(const Table& a, const Table& b) noexcept -> bool { return !(a == b); }
auto operator<<(std::ostream& os, const Table& self) -> std::ostream& {
    os << "Table { ";
    for (const auto& [key, value] : self.impl->value) {
        os << "[" << key << "] = " << value << ", ";
    }
    return os << " }";
}

// class CallContext
struct CallContext::Impl {
    Range location;
    Environment* env; // need to use pointer so we have move assignment operator
    Vallist args;
};
CallContext::CallContext(Environment* env)
    : impl(make_owning<Impl>(Impl{Range(), env, Vallist()})) {}
CallContext::CallContext(const CallContext& other) = default;
// NOLINTNEXTLINE
CallContext::CallContext(CallContext&& other) = default;
auto CallContext::operator=(const CallContext&) -> CallContext& = default;
// NOLINTNEXTLINE
auto CallContext::operator=(CallContext &&) -> CallContext& = default;
CallContext::~CallContext() = default;

auto CallContext::call_location() const -> Range { return impl->location; }
auto CallContext::environment() const -> Environment& { return *impl->env; }
auto CallContext::get(const std::string& name) const -> Value& { return impl->env->get(name); }
auto CallContext::arguments() const -> const Vallist& { return impl->args; }
auto CallContext::force_value(Value target, Value new_value) -> SourceChange {
    // TODO
    // needs some place to store source changes
    return {};
}

auto operator<<(std::ostream& os, const CallContext& self) -> std::ostream& {
    return os << "CallContext{ location = " << self.impl->location
              << ", environment = " << self.impl->env << ", arguments = " << self.impl->args
              << " }";
}

// class CallResult
CallResult::CallResult() { std::cout << "CallResult\n"; }
CallResult::CallResult(Vallist) { std::cout << "CallResult(Vallist)\n"; }
CallResult::CallResult(std::vector<Value> values) : CallResult(Vallist(values)) {}
CallResult::CallResult(std::initializer_list<Value> values) : CallResult(Vallist(values)) {}
CallResult::CallResult(SourceChange) { std::cout << "CallResult(SourceChange)\n"; }
CallResult::CallResult(Vallist, SourceChange) {
    std::cout << "CallResult(Vallist, SourceChange)\n";
}

// struct NativeFunction
auto operator<<(std::ostream& os, const NativeFunction & /*unused*/) -> std::ostream& {
    return os << "NativeFunction";
}
void swap(NativeFunction& self, NativeFunction& other) { std::swap(self.func, other.func); }

// class Value
struct Value::Impl {
    Type val;
    Origin origin;
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
// NOLINTNEXTLINE
Value::Value(Value&& other) = default;
Value::~Value() = default;
auto Value::operator=(const Value& other) -> Value& = default;
// NOLINTNEXTLINE
auto Value::operator=(Value&& other) -> Value& = default;
void swap(Value& self, Value& other) { std::swap(self.impl, other.impl); }

auto Value::get() -> Value::Type& { return impl->val; }
auto Value::get() const -> const Value::Type& { return impl->val; }

auto operator==(const Value& a, const Value& b) noexcept -> bool { return a.get() == b.get(); }
auto operator!=(const Value& a, const Value& b) noexcept -> bool { return !(a == b); }
auto operator<<(std::ostream& os, const Value& self) -> std::ostream& {
    std::visit([&](const auto& value) { os << "Value(" << value << ")"; }, self.get());
    return os;
}

auto Value::operator[](const Value& index) -> Value& {
    // TODO metatable for absent fields
    return std::visit(
        minilua::overloaded{
            [this](minilua::Table& index) -> Value& { return (*this)[index]; },
            [](auto & /*unused*/) -> Value& { throw std::runtime_error("unimplemented"); },
        },
        index.impl->val);
}
auto Value::operator[](const Value& index) const -> const Value& {
    return std::visit(
        overloaded{
            [this](Table& index) -> const Value& { return (*this)[index]; },
            [](auto & /*unused*/) -> const Value& { throw std::runtime_error("unimplemented"); },
        },
        index.impl->val);
}

#define IMPL_ARITHMETIC(OP, ERR_INFO)                                                              \
    auto operator OP(const Value& lhs, const Value& rhs)->Value {                                  \
        auto origin = Origin({BinaryOrigin{make_owning<Value>(lhs), make_owning<Value>(rhs)}});    \
        return std::visit(                                                                         \
            overloaded{                                                                            \
                [&origin](const Number& lhs, const Number& rhs) -> Value {                         \
                    auto value = Value(lhs OP rhs);                                                \
                    value.impl->origin = origin;                                                   \
                    return value;                                                                  \
                },                                                                                 \
                [](const Table& lhs, const Table& rhs) -> Value { /* NOLINT */                     \
                                                                  /* TODO tables with metatables   \
                                                                   */                              \
                                                                  throw std::runtime_error(        \
                                                                      "unimplemented");            \
                },                                                                                 \
                [](const auto& lhs, const auto& rhs) -> Value {                                    \
                    std::string msg = "Can not ";                                                  \
                    msg.append(ERR_INFO);                                                          \
                    msg.append(" values of type ");                                                \
                    msg.append(lhs.TYPE);                                                          \
                    msg.append(" and ");                                                           \
                    msg.append(rhs.TYPE);                                                          \
                    msg.append(".");                                                               \
                    throw std::runtime_error(msg);                                                 \
                }},                                                                                \
            lhs.impl->val, rhs.impl->val);                                                         \
    }

IMPL_ARITHMETIC(+, "add");
IMPL_ARITHMETIC(-, "subtract");
IMPL_ARITHMETIC(*, "multiply");
IMPL_ARITHMETIC(/, "divide");
IMPL_ARITHMETIC(^, "attempt to pow");
IMPL_ARITHMETIC(%, "take modulo of");

auto operator&(const Value& lhs, const Value& rhs) -> Value {
    // TODO
    return Value();
}
auto operator|(const Value& lhs, const Value& rhs) -> Value {
    // TODO
    return Value();
}
auto operator&&(const Value& lhs, const Value& rhs) -> Value {
    // TODO
    return Value();
}
auto operator||(const Value& lhs, const Value& rhs) -> Value {
    // TODO
    return Value();
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
auto std::hash<minilua::Bool>::operator()(const minilua::Bool& /*value*/) const -> size_t {
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
    // TODO maybe use address of shared_ptr directly
    return std::hash<decltype(value.func)>()(value.func);
}
} // namespace std

namespace minilua {

// class Vallist
struct Vallist::Impl {
    std::vector<Value> values;
};
Vallist::Vallist() { std::cout << "Vallist()\n"; }
Vallist::Vallist(std::vector<Value>) { std::cout << "Vallist(vector)\n"; }
Vallist::Vallist(std::initializer_list<Value>) { std::cout << "Vallist(<init-list>)\n"; }

Vallist::Vallist(const Vallist&) = default;
// NOLINTNEXTLINE
Vallist::Vallist(Vallist&&) = default;
auto Vallist::operator=(const Vallist&) -> Vallist& = default;
// NOLINTNEXTLINE
auto Vallist::operator=(Vallist &&) -> Vallist& = default;
Vallist::~Vallist() = default;

auto Vallist::size() const -> size_t { return impl->values.size(); }
auto Vallist::get(size_t index) const -> const Value& { return impl->values.at(index); }
auto Vallist::begin() const -> std::vector<Value>::const_iterator { return impl->values.cbegin(); }
auto Vallist::end() const -> std::vector<Value>::const_iterator { return impl->values.cend(); }

auto operator<<(std::ostream& os, const Vallist& self) -> std::ostream& {
    os << "Vallist{ ";
    for (const auto& value : self.impl->values) {
        os << value << ", ";
    }
    os << "}";
    return os;
}

} // namespace minilua
