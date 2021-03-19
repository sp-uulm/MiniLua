#include "MiniLua/values.hpp"
#include "MiniLua/environment.hpp"
#include "MiniLua/stdlib.hpp"
#include "MiniLua/utils.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

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

// class Number
[[nodiscard]] auto Number::to_literal() const -> std::string {
    // NOTE: use stringstream so we get better formatting than with std::to_string
    std::ostringstream result;
    this->visit(overloaded{
        [&result](Number::Int value) { result << value; },
        [&result](Number::Float value) {
            if (ceil(value) == value) {
                // always output the .0 for whole numbers
                result << std::setprecision(1) << std::fixed;
            }
            result << value;
        },
    });
    // TODO maybe we can use a better float representation algorithm than the default c++
    // (not sure what c++ uses by default or if it is the same on all compilers)
    return result.str();
}
auto operator<<(std::ostream& os, Number self) -> std::ostream& {
    os << "Number(";
    self.visit(overloaded{
        [&os](Number::Int value) { os << value; },
        [&os](Number::Float value) { os << value; },
    });
    return os << ")";
}
Number::operator bool() const { return true; }
auto Number::as_float() const -> Number::Float {
    return this->visit(overloaded{
        [](Number::Int value) { return static_cast<Number::Float>(value); },
        [](Number::Float value) { return value; },
    });
}
auto Number::try_as_int() const -> Number::Int {
    return this->visit(overloaded{
        [](Number::Int value) { return value; },
        [](Number::Float value) -> Number::Int {
            double num;
            double fraction = std::modf(value, &num);
            if (fraction != 0.0) {
                throw std::runtime_error(
                    std::string("number has no integer representation ") + std::to_string(value));
            } else {
                return (Number::Int)num;
            }
        },
    });
}
auto Number::convert_to_int() const -> Int {
    return this->visit(overloaded{
        [](Number::Int value) { return value; },
        [](Number::Float value) -> Number::Int { return static_cast<Number::Int>(value); },
    });
}
auto Number::raw() const -> std::variant<Int, Float> { return this->value; }
auto Number::is_int() const -> bool { return std::holds_alternative<Int>(this->value); }
auto Number::is_float() const -> bool { return std::holds_alternative<Float>(this->value); }

auto operator-(Number self) -> Number {
    return self.visit(overloaded{
        [](Number::Int value) { return Number(-value); },
        [](Number::Float value) { return Number(-value); },
    });
}

auto operator+(const Number& lhs, const Number& rhs) -> Number {
    return lhs.apply_with_number_rules(rhs, [](auto lhs, auto rhs) { return Number(lhs + rhs); });
}
auto operator-(const Number& lhs, const Number& rhs) -> Number {
    return lhs.apply_with_number_rules(rhs, [](auto lhs, auto rhs) { return Number(lhs - rhs); });
}
auto operator*(const Number& lhs, const Number& rhs) -> Number {
    return lhs.apply_with_number_rules(rhs, [](auto lhs, auto rhs) { return Number(lhs * rhs); });
}
auto operator/(const Number& lhs, const Number& rhs) -> Number {
    return lhs.as_float() / rhs.as_float();
}
auto Number::int_div(const Number& rhs) const -> Number {
    return static_cast<Number::Int>(this->as_float() / rhs.as_float());
}
auto Number::pow(const Number& rhs) const -> Number {
    return std::pow(this->as_float(), rhs.as_float());
}
auto Number::mod(const Number& rhs) const -> Number {
    return std::fmod(this->as_float(), rhs.as_float());
}
auto Number::bit_and(const Number& rhs) const -> Number {
    return this->try_as_int() & rhs.try_as_int();
}
auto Number::bit_or(const Number& rhs) const -> Number {
    return this->try_as_int() | rhs.try_as_int();
}
auto Number::bit_xor(const Number& rhs) const -> Number {
    return this->try_as_int() ^ rhs.try_as_int();
}
auto Number::bit_shl(const Number& rhs) const -> Number {
    return this->try_as_int() << rhs.try_as_int();
}
auto Number::bit_shr(const Number& rhs) const -> Number {
    return this->try_as_int() >> rhs.try_as_int();
}
auto Number::bit_not() const -> Number { return ~this->try_as_int(); }

auto operator==(Number lhs, Number rhs) noexcept -> bool {
    return lhs.apply_with_number_rules(rhs, [](auto lhs, auto rhs) { return lhs == rhs; });
}
auto operator!=(Number lhs, Number rhs) noexcept -> bool { return !(lhs == rhs); }
auto operator<(Number lhs, Number rhs) noexcept -> bool {
    return lhs.apply_with_number_rules(rhs, [](auto lhs, auto rhs) { return lhs < rhs; });
}
auto operator>(Number lhs, Number rhs) noexcept -> bool {
    return lhs.apply_with_number_rules(rhs, [](auto lhs, auto rhs) { return lhs > rhs; });
}
auto operator<=(Number lhs, Number rhs) noexcept -> bool {
    return lhs.apply_with_number_rules(rhs, [](auto lhs, auto rhs) { return lhs <= rhs; });
}
auto operator>=(Number lhs, Number rhs) noexcept -> bool {
    return lhs.apply_with_number_rules(rhs, [](auto lhs, auto rhs) { return lhs >= rhs; });
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

// class CallContext
struct CallContext::Impl {
    std::optional<Range> location;
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

[[nodiscard]] auto CallContext::make_new(Vallist args, std::optional<Range> location) const
    -> CallContext {
    CallContext new_cc{*this};
    new_cc.impl->args = std::move(args);
    new_cc.impl->location = location;
    return new_cc;
}

[[nodiscard]] auto CallContext::make_table() const -> Table {
    return this->environment().make_table();
}

auto CallContext::call_location() const -> std::optional<Range> { return impl->location; }
auto CallContext::environment() const -> Environment& { return *impl->env; }
auto CallContext::get(const std::string& name) const -> Value { return impl->env->get(name); }
auto CallContext::arguments() const -> const Vallist& { return impl->args; }

static auto
expect_number(const Value& value, std::optional<Range> call_location, const std::string& index)
    -> Value {
    auto number = value.to_number(Nil(), std::move(call_location));
    if (number.is_nil()) {
        // NOTE lua errors also mention the function name here but this is not
        // really necessary, because we will add that later
        throw std::runtime_error(
            "bad argument #" + index + " (number expected, got " + value.type() + ")");
    }
    return number;
}

[[nodiscard]] auto CallContext::unary_numeric_arg_helper() const
    -> std::tuple<Number, UnaryOrigin> {
    auto arg = this->arguments().get(0);

    auto num = expect_number(arg, this->call_location(), "1");

    // TODO not sure if we need to use arg or the result of calling to_number
    auto origin = UnaryOrigin{
        .val = std::make_shared<minilua::Value>(num),
        .location = this->call_location(),
    };

    return std::make_tuple(std::get<Number>(num), origin);
}

[[nodiscard]] auto CallContext::binary_numeric_args_helper() const
    -> std::tuple<Number, Number, BinaryOrigin> {
    auto arg1 = this->arguments().get(0);
    auto arg2 = this->arguments().get(1);

    auto num1 = expect_number(arg1, this->call_location(), "1");
    auto num2 = expect_number(arg2, this->call_location(), "2");

    // TODO not sure if we need to use arg1 and arg2 here or the results of
    // calling to_number
    auto origin = BinaryOrigin{
        .lhs = std::make_shared<Value>(num1),
        .rhs = std::make_shared<Value>(num2),
        .location = this->call_location(),
    };

    return std::make_tuple(std::get<Number>(num1), std::get<Number>(num2), origin);
}

auto operator<<(std::ostream& os, const CallContext& self) -> std::ostream& {
    os << "CallContext{ location = ";

    if (self.impl->location) {
        os << self.impl->location.value();
    } else {
        os << "nullopt";
    }
    os << ", environment = " << self.impl->env << ", arguments = " << self.impl->args << " }";
    return os;
}

// class CallResult
CallResult::CallResult() = default;
CallResult::CallResult(Vallist vallist) : vallist(std::move(vallist)) {}
CallResult::CallResult(std::vector<Value> values) : CallResult(Vallist(std::move(values))) {}
CallResult::CallResult(std::initializer_list<Value> values) : CallResult(Vallist(values)) {}
CallResult::CallResult(SourceChangeTree sc) : _source_change(sc) {}
CallResult::CallResult(std::optional<SourceChangeTree> sc) : _source_change(std::move(sc)) {}
CallResult::CallResult(Vallist vallist, SourceChangeTree sc)
    : vallist(std::move(vallist)), _source_change(sc) {}
CallResult::CallResult(Vallist vallist, std::optional<SourceChangeTree> sc)
    : vallist(std::move(vallist)), _source_change(std::move(sc)) {}

[[nodiscard]] auto CallResult::values() const -> const Vallist& { return this->vallist; }
[[nodiscard]] auto CallResult::source_change() const -> const std::optional<SourceChangeTree>& {
    return this->_source_change;
}

auto operator==(const CallResult& lhs, const CallResult& rhs) -> bool {
    return lhs.values() == rhs.values(); // && lhs.source_change() == rhs.source_change();
}
auto operator!=(const CallResult& lhs, const CallResult& rhs) -> bool { return !(lhs == rhs); }
auto operator<<(std::ostream& os, const CallResult& self) -> std::ostream& {
    os << "CallResult{ .values = " << self.values() << ", .source_change = ";
    if (self.source_change()) {
        os << *self.source_change();
    } else {
        os << "nullopt";
    }
    return os << " }";
}

// struct NativeFunction
auto operator<<(std::ostream& os, const Function& /*unused*/) -> std::ostream& {
    return os << "NativeFunction";
}
[[nodiscard]] auto Function::to_literal() const -> std::string {
    throw std::runtime_error("can't create a literal for a function");
}

auto Function::call(CallContext call_context) const -> CallResult {
    return (*this->func)(std::move(call_context));
}

Function::operator bool() const { return true; }
void swap(Function& self, Function& other) { std::swap(self.func, other.func); }

// class Origin
Origin::Origin() = default;
Origin::Origin(Type origin) : origin(std::move(origin)) {}
Origin::Origin(NoOrigin origin) : origin(origin) {}
Origin::Origin(ExternalOrigin origin) : origin(origin) {}
Origin::Origin(LiteralOrigin origin) : origin(origin) {}
Origin::Origin(BinaryOrigin origin) : origin(origin) {}
Origin::Origin(UnaryOrigin origin) : origin(origin) {}
Origin::Origin(MultipleArgsOrigin origin) : origin(origin) {}

[[nodiscard]] auto Origin::raw() const -> const Type& { return this->origin; }
auto Origin::raw() -> Type& { return this->origin; }

[[nodiscard]] auto Origin::is_none() const -> bool {
    return std::holds_alternative<NoOrigin>(this->raw());
}
[[nodiscard]] auto Origin::is_external() const -> bool {
    return std::holds_alternative<ExternalOrigin>(this->raw());
}
[[nodiscard]] auto Origin::is_literal() const -> bool {
    return std::holds_alternative<LiteralOrigin>(this->raw());
}
[[nodiscard]] auto Origin::is_binary() const -> bool {
    return std::holds_alternative<BinaryOrigin>(this->raw());
}
[[nodiscard]] auto Origin::is_unary() const -> bool {
    return std::holds_alternative<UnaryOrigin>(this->raw());
}

[[nodiscard]] auto Origin::force(const Value& new_value) const -> std::optional<SourceChangeTree> {
    return ::minilua::simplify(std::visit(
        overloaded{
            [&new_value](const BinaryOrigin& origin) -> std::optional<SourceChangeTree> {
                return origin.reverse(new_value, *origin.lhs, *origin.rhs);
            },
            [&new_value](const UnaryOrigin& origin) -> std::optional<SourceChangeTree> {
                return origin.reverse(new_value, *origin.val);
            },
            [&new_value](const MultipleArgsOrigin& origin) -> std::optional<SourceChangeTree> {
                return origin.reverse(new_value, origin.values);
            },
            [&new_value](const LiteralOrigin& origin) -> std::optional<SourceChangeTree> {
                return SourceChange(origin.location, new_value.to_literal());
            },
            [](const ExternalOrigin& /*unused*/) -> std::optional<SourceChangeTree> {
                return std::nullopt;
            },
            [](const NoOrigin& /*unused*/) -> std::optional<SourceChangeTree> {
                return std::nullopt;
            },
        },
        this->origin));
}
void Origin::set_file(std::optional<std::shared_ptr<std::string>> file) {
    std::visit(
        overloaded{
            [&file](BinaryOrigin& origin) {
                if (origin.location) {
                    origin.location->file = file;
                }
            },
            [&file](UnaryOrigin& origin) {
                if (origin.location) {
                    origin.location->file = file;
                }
            },
            [&file](LiteralOrigin& origin) { origin.location.file = file; },
            // TODO: fix formating
            [](NoOrigin& /*unused*/) {}, [](ExternalOrigin& /*unused*/) {}, [](auto /*unused*/) {}},
        this->origin);
}

auto Origin::simplify() const -> Origin {
    return std::visit([](const auto& origin) -> Origin { return origin.simplify(); }, this->raw());
}
[[nodiscard]] auto
Origin::with_updated_ranges(const std::unordered_map<Range, Range>& range_map) const -> Origin {
    return std::visit(
        overloaded{
            [&range_map](const LiteralOrigin& origin) -> Origin {
                for (const auto& [from, to] : range_map) {
                    if (from.start == origin.location.start && from.end == origin.location.end) {
                        auto location = to;
                        location.file = origin.location.file;
                        return LiteralOrigin{
                            .location = location,
                        };
                    }
                }
                return origin;
            },
            [&range_map](const BinaryOrigin& origin) -> Origin {
                auto lhs_origin = origin.lhs->origin().with_updated_ranges(range_map);
                auto rhs_origin = origin.rhs->origin().with_updated_ranges(range_map);

                auto new_origin = origin;
                new_origin.lhs = std::make_shared<Value>(origin.lhs->with_origin(lhs_origin));
                new_origin.rhs = std::make_shared<Value>(origin.rhs->with_origin(rhs_origin));
                return new_origin;
            },
            [&range_map](const UnaryOrigin& origin) -> Origin {
                auto val_origin = origin.val->origin().with_updated_ranges(range_map);

                auto new_origin = origin;
                new_origin.val = std::make_shared<Value>(origin.val->with_origin(val_origin));
                return new_origin;
            },
            [&range_map](const MultipleArgsOrigin& origin) -> Origin {
                std::vector<Value> new_values;
                new_values.reserve(origin.values.size());

                for (const auto& value : origin.values) {
                    auto new_origin = value.origin().with_updated_ranges(range_map);
                    new_values.push_back(value.with_origin(new_origin));
                }

                auto new_origin = origin;
                new_origin.values = new_values;
                return new_origin;
            },
            [&range_map](const auto& origin) -> Origin { return origin; },
        },
        this->origin);
}

auto operator==(const Origin& lhs, const Origin& rhs) noexcept -> bool {
    return lhs.raw() == rhs.raw();
}
auto operator!=(const Origin& lhs, const Origin& rhs) noexcept -> bool { return !(lhs == rhs); }
auto operator<<(std::ostream& os, const Origin& self) -> std::ostream& {
    os << "Origin(";
    std::visit([&os](const auto& inner) { os << inner; }, self.raw());
    return os << ")";
}

// struct NoOrigin
auto NoOrigin::simplify() const -> Origin { return *this; }

auto operator==(const NoOrigin&, const NoOrigin&) noexcept -> bool { return true; }
auto operator!=(const NoOrigin&, const NoOrigin&) noexcept -> bool { return false; }
auto operator<<(std::ostream& os, const NoOrigin&) -> std::ostream& { return os << "NoOrigin{}"; }

// struct ExternalOrigin
auto ExternalOrigin::simplify() const -> Origin { return *this; }

auto operator==(const ExternalOrigin&, const ExternalOrigin&) noexcept -> bool { return true; }
auto operator!=(const ExternalOrigin&, const ExternalOrigin&) noexcept -> bool { return true; }
auto operator<<(std::ostream& os, const ExternalOrigin&) -> std::ostream& {
    return os << "ExternalOrigin";
}

// struct LiteralOrigin
auto LiteralOrigin::simplify() const -> Origin { return *this; }

auto operator==(const LiteralOrigin& lhs, const LiteralOrigin& rhs) noexcept -> bool {
    return lhs.location == rhs.location;
}
auto operator!=(const LiteralOrigin& lhs, const LiteralOrigin& rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const LiteralOrigin& self) -> std::ostream& {
    return os << "LiteralOrigin(" << self.location << ")";
}

// struct BinaryOrigin
auto BinaryOrigin::simplify() const -> Origin {
    if (this->lhs->has_origin() && this->rhs->has_origin()) {
        return *this;
    } else {
        return NoOrigin();
    }
}

auto operator==(const BinaryOrigin& lhs, const BinaryOrigin& rhs) noexcept -> bool {
    return lhs.lhs == rhs.lhs && lhs.rhs == rhs.rhs && lhs.location == rhs.location;
}
auto operator!=(const BinaryOrigin& lhs, const BinaryOrigin& rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const BinaryOrigin& self) -> std::ostream& {
    os << "BinaryOrigin{ "
       << ".lhs = " << self.lhs << ", "
       << ".rhs = " << self.rhs << ", "
       << ".location = ";
    if (self.location) {
        os << self.location.value();
    } else {
        os << "nullopt";
    }
    os << ", .reverse = "
       << reinterpret_cast<void*>(self.reverse.target<BinaryOrigin::ReverseFn>());
    return os << " }";
}

// struct UnaryOrigin
auto UnaryOrigin::simplify() const -> Origin {
    if (this->val->has_origin()) {
        return *this;
    } else {
        return NoOrigin();
    }
}

auto operator==(const UnaryOrigin& lhs, const UnaryOrigin& rhs) noexcept -> bool {
    return lhs.val == rhs.val && lhs.location == rhs.location;
}
auto operator!=(const UnaryOrigin& lhs, const UnaryOrigin& rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const UnaryOrigin& self) -> std::ostream& {
    os << "UnaryOrigin{ "
       << ".val = " << self.val << ", "
       << ".location = ";
    if (self.location) {
        os << self.location.value();
    } else {
        os << "nullopt";
    }
    os << ", .reverse = " << reinterpret_cast<void*>(self.reverse.target<UnaryOrigin::ReverseFn>());
    os << " }";
    return os;
}

// struct MultipleArgsOrigin
auto MultipleArgsOrigin::simplify() const -> Origin {
    if (this->values.size() != 0 &&
        std::all_of(this->values.begin(), this->values.end(), [](const auto& value) {
            return value.has_origin();
        })) {
        return *this;
    } else {
        return NoOrigin();
    }
}

auto operator==(const MultipleArgsOrigin& lhs, const MultipleArgsOrigin& rhs) noexcept -> bool {
    return lhs.values == rhs.values && lhs.location == rhs.location;
}
auto operator!=(const MultipleArgsOrigin& lhs, const MultipleArgsOrigin& rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const MultipleArgsOrigin& self) -> std::ostream& {
    os << "MultipleArgsOrigin{ "
       << ".values = " << self.values << ", "
       << ".location = ";
    if (self.location) {
        os << self.location.value();
    } else {
        os << "nullopt";
    }
    os << ", .reverse = "
       << reinterpret_cast<void*>(self.reverse.target<MultipleArgsOrigin::ReverseFn>());
    os << " }";
    return os;
}

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
Value::Value(long val) : Value(Number(val)) {}
Value::Value(double val) : Value(Number(val)) {}
Value::Value(String val) : impl(make_owning<Impl>(Impl{.val = val})) {}
Value::Value(std::string val) : Value(String(std::move(val))) {}
Value::Value(const char* val) : Value(String(val)) {}
Value::Value(Table val) : impl(make_owning<Impl>(Impl{.val = val})) {}
Value::Value(Function val) : impl(make_owning<Impl>(Impl{.val = val})) {}

Value::Value(const Value& val, MemoryAllocator* allocator)
    : impl(make_owning<Impl>(Impl{
          .val = std::visit(
              overloaded{
                  [allocator](const Table& value) -> Value::Type {
                      return Table(value, allocator);
                  },
                  [](const auto& value) -> Value::Type { return value; }},
              val.raw())})) {}

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
            [](const auto& /*unused*/) -> bool { return false; },
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
    return std::holds_alternative<Function>(this->raw());
}
auto Value::contains_function() const -> bool {
    if (this->is_function()) {
        return true;
    } else if (this->is_table()) {
        return std::get<Table>(*this).contains_function();
    } else {
        return false;
    }
}

[[nodiscard]] auto Value::has_origin() const -> bool { return !this->impl->origin.is_none(); }

[[nodiscard]] auto Value::origin() const -> const Origin& { return this->impl->origin; }
[[nodiscard]] auto Value::origin() -> Origin& { return this->impl->origin; }

[[nodiscard]] auto Value::remove_origin() const -> Value {
    return this->with_origin(Origin{NoOrigin()});
}
[[nodiscard]] auto Value::with_origin(Origin new_origin) const -> Value {
    Value new_value{*this};
    new_value.impl->origin = std::move(new_origin);
    return new_value;
}

[[nodiscard]] auto Value::force(Value new_value, std::string origin) const
    -> std::optional<SourceChangeTree> {
    // TODO how to integrate origin string?
    return this->origin().force(new_value);
}

auto Value::call(CallContext call_context) const -> CallResult {
    return std::visit(
        overloaded{
            [&call_context](const Function& value) -> CallResult {
                return value.call(std::move(call_context));
            },
            // TODO tables with metatable with __call
            [](auto& /*unused*/) -> CallResult {
                throw std::runtime_error("can't call non function");
            },
        },
        this->raw());
}
auto Value::bind(CallContext call_context) const -> std::function<CallResult(Vallist)> {
    return std::visit(
        overloaded{
            [call_context = std::move(call_context)](
                const Function& value) -> std::function<CallResult(Vallist)> {
                return [call_context, &value](Vallist args) {
                    return value.call(call_context.make_new(std::move(args)));
                };
            },
            [](auto& /*unused*/) -> std::function<CallResult(Vallist)> {
                throw std::runtime_error("can't bind to a non function");
            },
        },
        this->raw());
}

auto Value::type() const -> std::string {
    return std::visit([](auto value) { return std::string(value.TYPE); }, this->raw());
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

auto Value::to_number(const Value base, std::optional<Range> location) const -> Value {
    return std::visit(
        overloaded{
            [this, &location](const String& number, const Nil& /*nil*/) -> Value {
                // same behaviour as number literal parsing but add a different origin
                auto origin = UnaryOrigin{
                    .val = std::make_shared<Value>(*this),
                    .location = location,
                    .reverse = [](const Value& new_value,
                                  const Value& old_value) -> std::optional<SourceChangeTree> {
                        if (new_value.is_number()) {
                            // TODO maybe produce the same format as the old value
                            return old_value.force(new_value.to_string());
                        } else {
                            return std::nullopt;
                        }
                    },
                };
                return parse_number_literal(number.value).with_origin(origin);
            },
            [this, base_value = base,
             &location](const String& number, const Number& base) -> Value {
                static std::regex to_number_int_pattern(R"(\s*-?[a-zA-Z0-9]+)");

                // NOTE: we only parse ints when we get a base
                // the base has to be between 2 adn 35 (because numbers with other bases
                // are not representable strings)
                if (!base.is_int()) {
                    throw std::runtime_error("base has no integer representation");
                }
                if (base < 2 || base > 36) { // NOLINT
                    throw std::runtime_error(
                        "base is to high or to low. base must be >= 2 and <= 36");
                }

                // number must be interpreted as an integer numeral in that base
                if (std::regex_match(number.value, to_number_int_pattern)) {
                    try {
                        auto origin = BinaryOrigin{
                            .lhs = std::make_shared<Value>(*this),
                            .rhs = std::make_shared<Value>(base_value),
                            .location = location,
                            .reverse =
                                [](const Value& new_value, const Value& old_lhs,
                                   const Value& old_base) -> std::optional<SourceChangeTree> {
                                if (new_value.is_number()) {
                                    // old_base is a number because otherwise to_number throws an
                                    // error
                                    Number base = std::get<Number>(old_base);
                                    Number new_number = std::get<Number>(new_value);
                                    if (new_number.is_int()) {
                                        return old_lhs.force(to_string_with_base(
                                            new_number.try_as_int(), base.try_as_int()));
                                    } else {
                                        return std::nullopt;
                                    }
                                } else {
                                    return std::nullopt;
                                }
                            }};

                        // try to parse as int and check if whole input is consumed
                        std::size_t last_pos;
                        Number::Int value = std::stoi(number.value, &last_pos, base.try_as_int());
                        if (last_pos != number.value.size()) {
                            return Nil();
                        }

                        return Value(value).with_origin(origin);
                    } catch (const std::invalid_argument& /*unused*/) {
                        // invalid base returns nil
                        return Nil();
                    }
                } else {
                    return Nil();
                }
            },
            [this](const Number& /*number*/, const Nil& /*unused*/) -> Value { return *this; },
            [](const auto& /*a*/, const auto& /*b*/) -> Value { return Nil(); }},
        this->raw(), base.raw());
}

auto Value::to_string(std::optional<Range> location) const -> Value {
    // TODO origin
    return std::visit(
        overloaded{
            [](Bool b) -> Value { return b.value ? "true" : "false"; },
            [](Number n) -> Value { return n.to_literal(); },
            [](const String& s) -> Value { return s.value; },
            [](Table t) -> Value { // TODO: maybe improve the way to get the address.
                // at the moment it could be that every time you call it the
                // address has changed because of the change in the stack
                ostringstream get_address;
                get_address << &t;
                return get_address.str();
            },
            [](Function f) -> Value {
                ostringstream get_address;
                get_address << &f;
                return get_address.str();
            },
            [](Nil /*nil*/) -> Value { return "nil"; }
            // TODO: add to_string for metatables
        },
        this->raw());
}

template <typename Fn, typename FnRev>
static inline auto num_op_helper(
    const Value& lhs, const Value& rhs, Fn op, std::string err_info, FnRev reverse,
    std::optional<Range> location) -> Value {
    static_assert(
        std::is_invocable_v<Fn, Number, Number>, "op is not invocable with two Number arguments");

    auto origin = Origin(BinaryOrigin{
        .lhs = std::make_shared<Value>(lhs),
        .rhs = std::make_shared<Value>(rhs),
        .location = location,
        .reverse = reverse,
    });

    return std::visit(
        overloaded{
            [op, &origin](const Number& lhs, const Number& rhs) -> Value {
                return Value(op(lhs, rhs)).with_origin(origin);
            },
            [&origin](const Table& lhs, const Table& rhs) -> Value {
                // TODO tables with metatables
                throw std::runtime_error("unimplemented");
            },
            [&err_info](const auto& lhs, const auto& rhs) -> Value {
                std::string msg = "Can not ";
                msg.append(err_info);
                msg.append(" values of type ");
                msg.append(lhs.TYPE);
                msg.append(" and ");
                msg.append(rhs.TYPE);
                msg.append(".");
                throw std::runtime_error(msg);
            }},
        lhs.raw(), rhs.raw());
}

[[nodiscard]] auto Value::negate(std::optional<Range> location) const -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = std::make_shared<Value>(*this),
        .location = location,
        .reverse = [](const Value& new_value, const Value& old_value)
            -> std::optional<SourceChangeTree> { return old_value.force(-new_value); }});

    return std::visit(
               overloaded{
                   [](const Number& value) -> Value { return -value; },
                   // TODO metatables maybe
                   [](const auto& value) -> Value {
                       std::string msg = "Can not negate values of type ";
                       msg.append(value.TYPE);
                       msg.append(".");
                       throw std::runtime_error(msg);
                   }},
               this->raw())
        .with_origin(origin);
}
[[nodiscard]] auto Value::add(const Value& rhs, std::optional<Range> location) const -> Value {
    return num_op_helper(
        *this, rhs, [](Number lhs, Number rhs) { return lhs + rhs; }, "add",
        binary_num_reverse(
            [](Number new_value, Number rhs) { return new_value - rhs; },
            [](Number new_value, Number lhs) { return new_value - lhs; }, "add"),
        location);
}
[[nodiscard]] auto Value::sub(const Value& rhs, std::optional<Range> location) const -> Value {
    return num_op_helper(
        *this, rhs, [](Number lhs, Number rhs) { return lhs - rhs; }, "subtract",
        binary_num_reverse(
            [](Number new_value, Number rhs) { return new_value + rhs; },
            [](Number new_value, Number lhs) { return lhs - new_value; }, "sub"),
        location);
}
[[nodiscard]] auto Value::mul(const Value& rhs, std::optional<Range> location) const -> Value {
    return num_op_helper(
        *this, rhs, [](Number lhs, Number rhs) { return lhs * rhs; }, "multiply",
        binary_num_reverse(
            [](Number new_value, Number rhs) { return new_value / rhs; },
            [](Number new_value, Number lhs) { return new_value / lhs; }, "mul"),
        location);
}
[[nodiscard]] auto Value::div(const Value& rhs, std::optional<Range> location) const -> Value {
    return num_op_helper(
        *this, rhs, [](Number lhs, Number rhs) { return lhs / rhs; }, "divide",
        binary_num_reverse(
            [](Number new_value, Number rhs) { return new_value * rhs; },
            [](Number new_value, Number lhs) { return lhs / new_value; }, "div"),
        location);
}
auto Value::int_div(const Value& rhs, std::optional<Range> location) const -> Value {
    return num_op_helper(
        *this, rhs, [](Number lhs, Number rhs) { return lhs.int_div(rhs); }, "int divide",
        binary_num_reverse(
            [](Number new_value, Number rhs) { return new_value * rhs; },
            [](Number new_value, Number lhs) { return lhs.int_div(new_value); }, "intdiv"),
        location);
}
[[nodiscard]] auto Value::pow(const Value& rhs, std::optional<Range> location) const -> Value {
    return num_op_helper(
        *this, rhs, [](Number lhs, Number rhs) { return lhs.pow(rhs); }, "attempt to pow",
        binary_num_reverse(
            [](Number new_value, Number rhs) {
                return std::pow(new_value.as_float(), 1 / rhs.as_float());
            },
            [](Number new_value, Number lhs) {
                return std::log(new_value.as_float()) / std::log(lhs.as_float());
            },
            "pow"),
        location);
}
[[nodiscard]] auto Value::mod(const Value& rhs, std::optional<Range> location) const -> Value {
    return num_op_helper(
        *this, rhs, [](Number lhs, Number rhs) { return lhs.mod(rhs); }, "take modulo of",
        [](auto...) { return std::nullopt; },
        location); // TODO reverse
}
[[nodiscard]] auto Value::bit_and(const Value& rhs, std::optional<Range> location) const -> Value {
    return num_op_helper(
        *this, rhs, [](Number lhs, Number rhs) { return lhs.bit_and(rhs); }, "bitwise and",
        [](auto...) { return std::nullopt; },
        location); // TODO reverse
}
[[nodiscard]] auto Value::bit_or(const Value& rhs, std::optional<Range> location) const -> Value {
    return num_op_helper(
        *this, rhs, [](Number lhs, Number rhs) { return lhs.bit_or(rhs); }, "bitwise or",
        [](auto...) { return std::nullopt; },
        location); // TODO reverse
}
auto Value::bit_xor(const Value& rhs, std::optional<Range> location) const -> Value {
    return num_op_helper(
        *this, rhs, [](Number lhs, Number rhs) { return lhs.bit_xor(rhs); }, "bitwise xor",
        [](auto...) { return std::nullopt; },
        location); // TODO reverse
}
auto Value::bit_shl(const Value& rhs, std::optional<Range> location) const -> Value {
    return num_op_helper(
        *this, rhs, [](Number lhs, Number rhs) { return lhs.bit_shl(rhs); }, "bitwise shift left",
        [](auto...) { return std::nullopt; },
        location); // TODO reverse
}
auto Value::bit_shr(const Value& rhs, std::optional<Range> location) const -> Value {
    return num_op_helper(
        *this, rhs, [](Number lhs, Number rhs) { return lhs.bit_shr(rhs); }, "bitwise shift right",
        [](auto...) { return std::nullopt; },
        location); // TODO reverse
}
auto Value::bit_not(std::optional<Range> location) const -> Value {
    return std::visit(
        overloaded{
            [this, &location](Number number) {
                auto origin = Origin(UnaryOrigin{
                    .val = std::make_shared<Value>(*this),
                    .location = location,
                    .reverse = [](const Value& new_value,
                                  const Value& old_value) -> std::optional<SourceChangeTree> {
                        if (old_value.is_number() && new_value.is_number()) {
                            Number new_number = std::get<Number>(new_value);
                            return old_value.force(new_number.bit_not());
                        }

                        return std::nullopt;
                    }});
                return Value(number.bit_not()).with_origin(origin);
            },
            [](const auto& value) -> Value {
                throw std::runtime_error(
                    "attempt to get bitwise not for value of type " + std::string(value.TYPE));
            },
        },
        this->raw());
}

[[nodiscard]] auto Value::logic_and(const Value& rhs, std::optional<Range> location) const
    -> Value {
    // return lhs if it is falsey and rhs otherwise
    auto origin = Origin(BinaryOrigin{
        .lhs = std::make_shared<Value>(*this),
        .rhs = std::make_shared<Value>(rhs),
        .location = location,
        .reverse = [](const Value& new_value, const Value& old_lhs,
                      const Value& old_rhs) -> std::optional<SourceChangeTree> {
            // will not intentially change which side is returned from the expression
            if (!old_lhs) {
                return old_lhs.force(new_value);
            } else {
                return old_rhs.force(new_value);
            }
        }});

    if (!*this) {
        return this->with_origin(origin);
    } else {
        return rhs.with_origin(origin);
    }
}
[[nodiscard]] auto Value::logic_or(const Value& rhs, std::optional<Range> location) const -> Value {
    // return lhs if it is truthy and rhs otherwise
    auto origin = Origin(BinaryOrigin{
        .lhs = std::make_shared<Value>(*this),
        .rhs = std::make_shared<Value>(rhs),
        .location = location,
        .reverse = [](const Value& new_value, const Value& old_lhs,
                      const Value& old_rhs) -> std::optional<SourceChangeTree> {
            // will not intentially change which side is returned from the expression
            if (old_lhs) {
                return old_lhs.force(new_value);
            } else {
                return old_rhs.force(new_value);
            }
        }});

    if (*this) {
        return this->with_origin(origin);
    } else {
        return rhs.with_origin(origin);
    }
}
[[nodiscard]] auto Value::invert(std::optional<Range> location) const -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = std::make_shared<Value>(*this),
        .location = location,
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            const Value negated_new_value = !bool(new_value);
            if (!old_value.is_bool() || !new_value.is_bool() || negated_new_value == old_value) {
                return std::nullopt;
            }

            if (old_value) { // true -> origin value was false
                return old_value.force(negated_new_value);
            } else { // false -> origin value was true
                return old_value.force(negated_new_value);
            }
        }});

    return Value(!bool(*this)).with_origin(origin);
}
[[nodiscard]] auto Value::len(std::optional<Range> location) const -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = std::make_shared<Value>(*this),
        .location = location,
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            // can't reverse the operation in almost all cases
            // TODO implement the few that could be reversed
            return std::nullopt;
        }});

    return std::visit(
               overloaded{
                   [](const String& value) -> Value { return (int)value.value.size(); },
                   [](const Table& value) -> Value { return value.border(); },
                   [](const auto& value) -> Value {
                       throw std::runtime_error(
                           "attempt to get length for value of type " + std::string(value.TYPE));
                   }},
               this->raw())
        .with_origin(origin);
}
[[nodiscard]] auto Value::equals(const Value& rhs, std::optional<Range> location) const -> Value {
    auto origin = Origin(BinaryOrigin{
        .lhs = std::make_shared<Value>(*this),
        .rhs = std::make_shared<Value>(rhs),
        .location = location,
        .reverse = [](const Value& new_value, const Value& old_lhs,
                      const Value& old_rhs) -> std::optional<SourceChangeTree> {
            // can't reverse the operation in almost all cases
            // TODO implement the few that could be reversed
            return std::nullopt;
        }});

    return Value(*this == rhs).with_origin(origin);
}
[[nodiscard]] auto Value::unequals(const Value& rhs, std::optional<Range> location) const -> Value {
    auto origin = Origin(BinaryOrigin{
        .lhs = std::make_shared<Value>(*this),
        .rhs = std::make_shared<Value>(rhs),
        .location = location,
        .reverse = [](const Value& new_value, const Value& old_lhs,
                      const Value& old_rhs) -> std::optional<SourceChangeTree> {
            // can't reverse the operation in almost all cases
            // TODO implement the few that could be reversed
            return std::nullopt;
        }});

    return Value(*this != rhs).with_origin(origin);
}
[[nodiscard]] auto Value::less_than(const Value& rhs, std::optional<Range> location) const
    -> Value {
    auto origin = Origin(BinaryOrigin{
        .lhs = std::make_shared<Value>(*this),
        .rhs = std::make_shared<Value>(rhs),
        .location = location,
        .reverse = [](const Value& new_value, const Value& old_lhs,
                      const Value& old_rhs) -> std::optional<SourceChangeTree> {
            // can't reverse the operation in almost all cases
            // TODO implement the few that could be reversed
            return std::nullopt;
        }});

    return std::visit(
               overloaded{
                   [](const Number& lhs, const Number& rhs) -> Value { return lhs < rhs; },
                   [](const String& lhs, const String& rhs) -> Value {
                       return lhs.value < rhs.value;
                   },
                   [](const auto& lhs, const auto& rhs) -> Value {
                       throw std::runtime_error(
                           "attempt to less than compare values of type " + std::string(lhs.TYPE) +
                           " and " + std::string(rhs.TYPE));
                   }},
               this->raw(), rhs.raw())
        .with_origin(origin);
}
[[nodiscard]] auto Value::less_than_or_equal(const Value& rhs, std::optional<Range> location) const
    -> Value {
    auto origin = Origin(BinaryOrigin{
        .lhs = std::make_shared<Value>(*this),
        .rhs = std::make_shared<Value>(rhs),
        .location = location,
        .reverse = [](const Value& new_value, const Value& old_lhs,
                      const Value& old_rhs) -> std::optional<SourceChangeTree> {
            // can't reverse the operation in almost all cases
            // TODO implement the few that could be reversed
            return std::nullopt;
        }});

    return std::visit(
               overloaded{
                   [](const Number& lhs, const Number& rhs) -> Value { return lhs <= rhs; },
                   [](const String& lhs, const String& rhs) -> Value {
                       return lhs.value <= rhs.value;
                   },
                   [](const auto& lhs, const auto& rhs) -> Value {
                       throw std::runtime_error(
                           "attempt to less than or equal compare values of type " +
                           std::string(lhs.TYPE) + " and " + std::string(rhs.TYPE));
                   }},
               this->raw(), rhs.raw())
        .with_origin(origin);
}
[[nodiscard]] auto Value::greater_than(const Value& rhs, std::optional<Range> location) const
    -> Value {
    auto origin = Origin(BinaryOrigin{
        .lhs = std::make_shared<Value>(*this),
        .rhs = std::make_shared<Value>(rhs),
        .location = location,
        .reverse = [](const Value& new_value, const Value& old_lhs,
                      const Value& old_rhs) -> std::optional<SourceChangeTree> {
            // can't reverse the operation in almost all cases
            // TODO implement the few that could be reversed
            return std::nullopt;
        }});

    return std::visit(
               overloaded{
                   [](const Number& lhs, const Number& rhs) -> Value { return lhs > rhs; },
                   [](const String& lhs, const String& rhs) -> Value {
                       return lhs.value > rhs.value;
                   },
                   [](const auto& lhs, const auto& rhs) -> Value {
                       throw std::runtime_error(
                           "attempt to greater than compare values of type " +
                           std::string(lhs.TYPE) + " and " + std::string(rhs.TYPE));
                   }},
               this->raw(), rhs.raw())
        .with_origin(origin);
}
[[nodiscard]] auto
Value::greater_than_or_equal(const Value& rhs, std::optional<Range> location) const -> Value {
    auto origin = Origin(BinaryOrigin{
        .lhs = std::make_shared<Value>(*this),
        .rhs = std::make_shared<Value>(rhs),
        .location = location,
        .reverse = [](const Value& new_value, const Value& old_lhs,
                      const Value& old_rhs) -> std::optional<SourceChangeTree> {
            // can't reverse the operation in almost all cases
            // TODO implement the few that could be reversed
            return std::nullopt;
        }});

    return std::visit(
               overloaded{
                   [](const Number& lhs, const Number& rhs) -> Value { return lhs >= rhs; },
                   [](const String& lhs, const String& rhs) -> Value {
                       return lhs.value >= rhs.value;
                   },
                   [](const auto& lhs, const auto& rhs) -> Value {
                       throw std::runtime_error(
                           "attempt to greater than or equal compare values of type " +
                           std::string(lhs.TYPE) + " and " + std::string(rhs.TYPE));
                   }},
               this->raw(), rhs.raw())
        .with_origin(origin);
}
[[nodiscard]] auto Value::concat(const Value& rhs, std::optional<Range> location) const -> Value {
    auto origin = Origin(BinaryOrigin{
        .lhs = std::make_shared<Value>(*this),
        .rhs = std::make_shared<Value>(rhs),
        .location = location,
        .reverse = [](const Value& new_value, const Value& old_lhs,
                      const Value& old_rhs) -> std::optional<SourceChangeTree> {
            // can't reverse the operation in almost all cases
            // TODO implement the few that could be reversed
            return std::nullopt;
        }});

    return std::visit(
               overloaded{
                   [](const String& lhs, const String& rhs) -> Value {
                       return lhs.value + rhs.value;
                   },
                   [](const String& lhs, const Number& rhs) -> Value {
                       Environment env;
                       // TODO use the original value to correctly track the origin
                       return lhs.value + Value(rhs).to_string().remove_origin();
                   },
                   [](const Number& lhs, const String& rhs) -> Value {
                       Environment env;
                       return Value(lhs).to_string().remove_origin() + rhs.value;
                   },
                   [](const Number& lhs, const Number& rhs) -> Value {
                       Environment env;
                       return Value(lhs).to_string().remove_origin() +
                              Value(rhs).to_string().remove_origin();
                   },
                   [](const auto& lhs, const auto& rhs) -> Value {
                       throw std::runtime_error(
                           "attempt to concatenate values of type " + std::string(lhs.TYPE) +
                           " and " + std::string(rhs.TYPE));
                   }},
               this->raw(), rhs.raw())
        .with_origin(origin);
}

// arithmetic operators
auto operator-(const Value& value) -> Value { return value.negate(); }
auto operator+(const Value& lhs, const Value& rhs) -> Value { return lhs.add(rhs); }
auto operator-(const Value& lhs, const Value& rhs) -> Value { return lhs.sub(rhs); }
auto operator*(const Value& lhs, const Value& rhs) -> Value { return lhs.mul(rhs); }
auto operator/(const Value& lhs, const Value& rhs) -> Value { return lhs.div(rhs); }
auto operator^(const Value& lhs, const Value& rhs) -> Value { return lhs.pow(rhs); }
auto operator%(const Value& lhs, const Value& rhs) -> Value { return lhs.mod(rhs); }
// bitwise operators
auto operator&(const Value& lhs, const Value& rhs) -> Value { return lhs.bit_and(rhs); }
auto operator|(const Value& lhs, const Value& rhs) -> Value { return lhs.bit_or(rhs); }
// logic operators
auto operator&&(const Value& lhs, const Value& rhs) -> Value { return lhs.logic_and(rhs); }
auto operator||(const Value& lhs, const Value& rhs) -> Value { return lhs.logic_or(rhs); }
auto operator!(const Value& value) -> Value { return value.invert(); }

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
    // we treat whole floats like their integer equivalent

    // NOTE lua does not allow using NaN as a table key but we are not allowed
    // to throw inside of std::hash
    return std::visit(
        minilua::overloaded{
            [](minilua::Number::Int value) { return std::hash<minilua::Number::Int>()(value); },
            [](minilua::Number::Float value) {
                if (std::isnan(value) || std::isinf(value)) {
                    return std::numeric_limits<size_t>::max();
                }

                if (ceil(value) == value) {
                    return std::hash<minilua::Number::Int>()(
                        static_cast<minilua::Number::Int>(value));
                }

                return std::hash<minilua::Number::Float>()(value);
            }},
        value.raw());
}
auto std::hash<minilua::String>::operator()(const minilua::String& value) const -> size_t {
    return std::hash<std::string>()(value.value);
}
auto std::hash<minilua::Table>::operator()(const minilua::Table& value) const -> size_t {
    return std::hash<decltype(value.impl)>()(value.impl);
}
auto std::hash<minilua::Function>::operator()(const minilua::Function& value) const -> size_t {
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
Vallist::Vallist(Value value) : Vallist({std::move(value)}) {}
Vallist::Vallist(std::vector<Value> values)
    : impl(make_owning<Vallist::Impl>(Vallist::Impl{std::move(values)})) {}
Vallist::Vallist(std::initializer_list<Value> values)
    : impl(make_owning<Vallist::Impl>(
          Vallist::Impl{std::vector<Value>(values.begin(), values.end())})) {}

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

auto operator==(const Vallist& lhs, const Vallist& rhs) -> bool {
    return lhs.impl->values == rhs.impl->values;
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
