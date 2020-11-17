#include "MiniLua/values.hpp"

#include <cmath>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

namespace minilua {

// struct Nil
[[nodiscard]] auto Nil::to_literal() const -> std::string { return "nil"; }
auto operator<<(std::ostream& os, Nil /*unused*/) -> std::ostream& { return os << "Nil"; }
Nil::operator bool() const { return false; }

static const Value GLOBAL_NIL_CONST = Nil();

// struct Bool
[[nodiscard]] auto Bool::to_literal() const -> std::string {
    if (this->value) {
        return "true";
    } else {
        return "false";
    }
}
auto operator<<(std::ostream& os, Bool self) -> std::ostream& {
    return os << "Bool(" << std::boolalpha << self.value << std::noboolalpha << ")";
}
Bool::operator bool() const { return this->value; }

// struct Number
[[nodiscard]] auto Number::to_literal() const -> std::string {
    // NOTE: use stringstream so we get better formatting than with std::to_string
    std::ostringstream result;
    if (this->value == std::floor(this->value)) {
        result << static_cast<long>(this->value);
    } else {
        // TODO maybe we can use a better float representation algorithm than the default c++
        // (not sure what c++ uses by default)

        result << this->value;
    }
    return result.str();
}
auto operator<<(std::ostream& os, Number self) -> std::ostream& {
    return os << "Number(" << self.value << ")";
}
Number::operator bool() const { return true; }
auto operator^(Number lhs, Number rhs) -> Number { return std::pow(lhs.value, rhs.value); }
auto operator%(Number lhs, Number rhs) -> Number { return Number(std::fmod(lhs.value, rhs.value)); }
auto operator&(Number lhs, Number rhs) -> Number {
    if (lhs.value != std::floor(lhs.value)) {
        throw std::runtime_error("lhs of bitwise and is not an integer");
    }
    if (rhs.value != std::floor(rhs.value)) {
        throw std::runtime_error("rhs of bitwise and is not an integer");
    }
    // TODO is int big enough or do we need long?
    int lhs_int = lhs.value;
    int rhs_int = rhs.value;

    return Number(lhs_int & rhs_int);
}
auto operator|(Number lhs, Number rhs) -> Number {
    if (lhs.value != std::floor(lhs.value)) {
        throw std::runtime_error("lhs of bitwise or is not an integer");
    }
    if (rhs.value != std::floor(rhs.value)) {
        throw std::runtime_error("rhs of bitwise or is not an integer");
    }
    // TODO is int big enough or do we need long?
    int lhs_int = lhs.value;
    int rhs_int = rhs.value;

    return Number(lhs_int | rhs_int);
}

// helper function to escape characters in string literals
auto escape_char(char c) -> std::string {
    if (0 <= c && c <= 31) { // NOLINT
        switch (c) {
        case 7: // NOLINT
            return "\\a";
        case 8: // NOLINT
            return "\\b";
        case 9: // NOLINT
            return "\\t";
        case 10: // NOLINT
            return "\\n";
        case 11: // NOLINT
            return "\\v";
        case 12: // NOLINT
            return "\\f";
        case 13: // NOLINT
            return "\\r";
        default:
            // format as \000
            std::array<char, 4> escaped;
            std::snprintf(escaped.data(), 4, "%3d", c);
            std::string escaped_str;
            escaped_str.reserve(4);
            escaped_str.append("\\");
            escaped_str.append(escaped.data());
            return escaped_str;
        }
    } else if (c == '"') {
        return "\\\"";
    } else if (c == '\\') {
        return "\\\\";
    } else {
        return std::string(1, c);
    }
}

// struct String
String::String(std::string value) : value(std::move(value)) {}

[[nodiscard]] auto String::to_literal() const -> std::string {
    // TODO this can probably be implemented more efficiently (and more clearly)

    std::string str;
    str.reserve(this->value.size() + 2);

    // characters to replace
    // 7 -> \a (bell)
    // 8 -> \b (back space)
    // 9 -> \t (horizontal tab)
    // 10 -> \n (line feed)
    // 11 -> \v (vertical tab)
    // 12 -> \f (form feed)
    // 13 -> \r (cariage return)
    // \ -> \\
    // " -> \"
    // ' -> \' (not needed if string surrounded by ")
    // invisible chars -> \000

    str.append("\"");

    for (char c : this->value) {
        str.append(escape_char(c));
    }

    str.append("\"");

    return str;
}

[[nodiscard]] auto String::is_valid_identifier() const -> bool {
    // According to the lua spec:
    // Names (also called identifiers) in Lua can be any string of letters,
    // digits, and underscores, not beginning with a digit.
    static std::regex regex{R"(^[A-Za-z_][\w_]*$)"};
    return std::regex_search(this->value, regex);
}

String::operator bool() const { return true; }

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

[[nodiscard]] auto Table::to_literal() const -> std::string {
    // NOTE: recursive table check needs to be in a lambda because Table::Impl is private and we
    // don't want a helper function in the public interface
    std::set<Table::Impl*> visited;
    auto table_to_literal = [&visited](const Table& table, const auto& rec) -> std::string {
        visited.insert(table.impl.get());
        auto visit_nested = [&rec, &visited](const Value& value) -> std::string {
            return std::visit(
                overloaded{
                    [&visited, &rec](const Table& nested) -> std::string {
                        if (visited.find(nested.impl.get()) != visited.end()) {
                            throw std::runtime_error(
                                "self recursive table can't be converted to literal");
                        }
                        return rec(nested, rec);
                    },
                    [](const auto& nested) -> std::string { return nested.to_literal(); }},
                value.raw());
        };

        // TODO should we sort keys for consistency?
        std::string str;
        str.append("{");

        const char* sep = " ";

        for (const auto& [key, value] : table.impl->value) {
            if (value.is_nil()) {
                continue;
            }

            str.append(sep);

            // use strings directly as identifiers if possible
            if (key.is_valid_identifier()) {
                str.append(std::get<String>(key.raw()).value);
            } else {
                str.append("[");
                str.append(visit_nested(key));
                str.append("]");
            }

            str.append(" = ");
            str.append(visit_nested(value));
            sep = ", ";
        }

        if (!table.impl->value.empty()) {
            str.append(" ");
        }

        str.append("}");
        return str;
    };

    return table_to_literal(*this, table_to_literal);
}

auto Table::operator[](const Value& index) -> Value& { return impl->value[index]; }
auto Table::operator[](const Value& index) const -> const Value& { return impl->value[index]; }

Table::operator bool() const { return true; }

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
auto CallContext::operator=(CallContext&&) -> CallContext& = default;
CallContext::~CallContext() = default;

auto CallContext::call_location() const -> Range { return impl->location; }
auto CallContext::environment() const -> Environment& { return *impl->env; }
auto CallContext::get(const std::string& name) const -> Value& { return impl->env->get(name); }
auto CallContext::arguments() const -> const Vallist& { return impl->args; }

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
auto operator<<(std::ostream& os, const NativeFunction& /*unused*/) -> std::ostream& {
    return os << "NativeFunction";
}
[[nodiscard]] auto NativeFunction::to_literal() const -> std::string {
    throw std::runtime_error("can't create a literal for a function");
}
NativeFunction::operator bool() const { return true; }
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

auto Value::raw() -> Value::Type& { return impl->val; }
auto Value::raw() const -> const Value::Type& { return impl->val; }

[[nodiscard]] auto Value::to_literal() const -> std::string {
    return std::visit([](auto value) -> std::string { return value.to_literal(); }, this->raw());
}

[[nodiscard]] auto Value::is_valid_identifier() const -> bool {
    return std::visit(
        overloaded{
            [](const String& value) -> bool { return value.is_valid_identifier(); },
            [](const auto&) -> bool { return false; },
        },
        this->raw());
}

[[nodiscard]] auto Value::is_nil() const -> bool {
    return std::holds_alternative<Nil>(this->raw());
}
[[nodiscard]] auto Value::is_bool() const -> bool {
    return std::holds_alternative<Bool>(this->raw());
}
[[nodiscard]] auto Value::is_number() const -> bool {
    return std::holds_alternative<Number>(this->raw());
}
[[nodiscard]] auto Value::is_string() const -> bool {
    return std::holds_alternative<String>(this->raw());
}
[[nodiscard]] auto Value::is_table() const -> bool {
    return std::holds_alternative<Table>(this->raw());
}
[[nodiscard]] auto Value::is_function() const -> bool {
    return std::holds_alternative<NativeFunction>(this->raw());
}

auto Value::force(Value new_value, std::string origin) -> SourceChange {
    // TODO force value
    return SourceChange();
}

auto operator==(const Value& a, const Value& b) noexcept -> bool { return a.raw() == b.raw(); }
auto operator!=(const Value& a, const Value& b) noexcept -> bool { return !(a == b); }
auto operator<<(std::ostream& os, const Value& self) -> std::ostream& {
    std::visit([&](const auto& value) { os << "Value(" << value << ")"; }, self.raw());
    return os;
}

auto Value::operator[](const Value& index) -> Value& {
    // TODO metatable for absent fields
    return std::visit(
        minilua::overloaded{
            [&index](Table& value) -> Value& { return value[index]; },
            [](auto& value) -> Value& {
                throw std::runtime_error("can't index into " + std::string(value.TYPE));
            },
        },
        this->impl->val);
}
auto Value::operator[](const Value& index) const -> const Value& {
    return std::visit(
        overloaded{
            [&index](const Table& value) -> const Value& { return value[index]; },
            [](const auto& value) -> const Value& {
                throw std::runtime_error("can't index into " + std::string(value.TYPE));
            },
        },
        this->impl->val);
}

Value::operator bool() const {
    return std::visit([](const auto& value) { return bool(value); }, this->impl->val);
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
IMPL_ARITHMETIC(&, "bitwise and");
IMPL_ARITHMETIC(|, "bitwise or");

// logic operators
auto operator&&(const Value& lhs, const Value& rhs) -> Value {
    // return lhs if it is falsey and rhs otherwise
    auto origin = Origin({BinaryOrigin{make_owning<Value>(lhs), make_owning<Value>(rhs)}});
    if (!lhs) {
        auto value = Value(lhs);
        value.impl->origin = origin;
        return value;
    } else {
        auto value = Value(rhs);
        value.impl->origin = origin;
        return value;
    }
}
auto operator||(const Value& lhs, const Value& rhs) -> Value {
    // return lhs if it is truthy and rhs otherwise
    auto origin = Origin({BinaryOrigin{make_owning<Value>(lhs), make_owning<Value>(rhs)}});
    if (lhs) {
        auto value = Value(lhs);
        value.impl->origin = origin;
        return value;
    } else {
        auto value = Value(rhs);
        value.impl->origin = origin;
        return value;
    }
}

} // namespace minilua

namespace std {
auto std::hash<minilua::Value>::operator()(const minilua::Value& value) const -> size_t {
    return std::hash<minilua::Value::Type>()(value.raw());
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
Vallist::Vallist() = default;
Vallist::Vallist(std::vector<Value> values)
    : impl(make_owning<Vallist::Impl>(Vallist::Impl{std::move(values)})) {}
Vallist::Vallist(std::initializer_list<Value> values)
    : impl(make_owning<Vallist::Impl>(Vallist::Impl{std::vector(values.begin(), values.end())})) {}

Vallist::Vallist(const Vallist&) = default;
// NOLINTNEXTLINE
Vallist::Vallist(Vallist&&) = default;
auto Vallist::operator=(const Vallist&) -> Vallist& = default;
// NOLINTNEXTLINE
auto Vallist::operator=(Vallist&&) -> Vallist& = default;
Vallist::~Vallist() = default;

auto Vallist::size() const -> size_t { return impl->values.size(); }
auto Vallist::get(size_t index) const -> const Value& {
    if (impl->values.size() > index) {
        return impl->values.at(index);
    } else {
        return GLOBAL_NIL_CONST;
    }
}

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
