#include <cmath>
#include <iostream>
#include <random>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "MiniLua/math.hpp"
#include "MiniLua/source_change.hpp"
#include "MiniLua/stdlib.hpp"
#include "MiniLua/utils.hpp"
#include "MiniLua/values.hpp"

namespace minilua::math {

std::default_random_engine random_seed;

// functions to reduce the duplicate code because almost every math-function does the same thing
// except the function that is called to determine the new value
// gets 1 parameter
auto static math_helper(
    const CallContext& ctx, const std::function<double(double)>& function,
    const std::string& functionname, const std::string& arg_index = "1") -> Value {
    auto res = to_number(ctx);

    if (res != Nil()) {
        auto num = std::get<Number>(res);

        return Value(function(num.value));
    } else {
        auto x = ctx.arguments().get(0);
        throw std::runtime_error(
            "bad argument #" + arg_index + " to '" + functionname + "' (number expected, got " +
            x.type() + ")");
    }
}

// gets 2 parameter
template <class T>
auto static math_helper(
    const CallContext& ctx, const std::function<T(double, double)>& function,
    const std::string& functionname) -> Value {

    Vallist list = Vallist({ctx.arguments().get(0)});
    CallContext new_ctx = ctx.make_new(list);
    auto res1 = to_number(new_ctx);

    if (res1 != Nil()) {
        list = Vallist({ctx.arguments().get(1)});
        new_ctx = ctx.make_new(list);
        auto res2 = to_number(new_ctx);

        if (res2 != Nil()) {
            auto num1 = std::get<Number>(res1);
            auto num2 = std::get<Number>(res2);

            return Value(function(num1.value, num2.value));
        } else {
            auto y = ctx.arguments().get(1);
            throw std::runtime_error(
                "bad argument #2 to '" + functionname + "' (number expected, got " + y.type() +
                ")");
        }
    } else {
        auto x = ctx.arguments().get(0);
        throw std::runtime_error(
            "bad argument #1 to '" + functionname + "' (number expected, got " + x.type() + ")");
    }
}

// All math-functions can be called be a number or a string that is formated like a number.
// This information is not documented in the documentation. I just found that when I was testing the
// error behaviour for wrong input of the function in lua
// That is the reason why everywhere to_number is called.

auto abs(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(Nil()); // TODO: add real return value
        }});
    return math_helper(ctx, static_cast<double (*)(double)>(&std::abs), "abs");
}

auto acos(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(std::cos(n.value));
        }});

    return math_helper(ctx, static_cast<double (*)(double)>(&std::acos), "acos")
        .with_origin(origin);
}

auto asin(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(std::sin(n.value));
        }});
    return math_helper(ctx, static_cast<double (*)(double)>(&std::asin), "asin")
        .with_origin(origin);
}

auto atan(const CallContext& ctx) -> Value {
    auto origin = Origin(BinaryOrigin{
        .lhs = make_owning<Value>(ctx.arguments().get(0)),
        .rhs = make_owning<Value>(ctx.arguments().get(1)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value, const Value& old_value1,
                      const Value& old_value2) -> std::optional<SourceChangeTree> {
            if (old_value2 == Nil()) {
                Number n = std::get<Number>(new_value);
                return old_value1.force(std::tan(n.value));
            } else {
                Number n = std::get<Number>(new_value);
                return old_value1.force(Nil()); // TODO: add real return
            }
        }});

    auto x = ctx.arguments().get(0);
    auto y = ctx.arguments().get(1);

    return std::visit(
               overloaded{
                   [](Number x, Number y) { return Value(std::atan2(x.value, y.value)); },
                   [ctx](Number x, String y) {
                       Vallist list = Vallist({Value(std::move(y))});
                       CallContext new_ctx = ctx.make_new(list);
                       auto res = to_number(new_ctx);

                       if (res != Nil()) {
                           auto num = get<Number>(res);

                           return Value(std::atan2(x.value, num.value));
                       } else {
                           throw std::runtime_error(
                               "bad argument #2 to 'atan' (number expected, got string)");
                       }
                   },
                   [](Number x, Nil /*unused*/) { return Value(std::atan2(x.value, 1)); },
                   [](Number /*unused*/, auto y) -> Value {
                       throw std::runtime_error(
                           "bad argument #2 to 'atan' (number expected, got " +
                           std::string(y.TYPE) + ")");
                   },
                   [ctx](String x, Number y) {
                       Vallist list = Vallist({Value(std::move(x))});
                       CallContext new_ctx = ctx.make_new(list);
                       auto res = to_number(new_ctx);

                       if (res != Nil()) {
                           auto num = get<Number>(res);

                           return Value(std::atan2(num.value, y.value));
                       } else {
                           throw std::runtime_error(
                               "bad argument #1 to 'atan' (number expected, got string)");
                       }
                   },
                   [ctx](String x, String y) {
                       Vallist list = Vallist({Value(std::move(x))});
                       CallContext new_ctx = ctx.make_new(list);
                       auto res1 = to_number(new_ctx);

                       if (res1 != Nil()) {
                           list = Vallist({Value(std::move(y))});
                           new_ctx = ctx.make_new(list);
                           auto res2 = to_number(new_ctx);

                           if (res2 != Nil()) {
                               auto x1 = std::get<Number>(res1);
                               auto x2 = std::get<Number>(res2);

                               return Value(std::atan2(x1.value, x2.value));
                           } else {
                               throw std::runtime_error(
                                   "bad argument #2 to 'atan' (number expected, got string)");
                           }
                       } else {
                           throw std::runtime_error(
                               "bad argument #1 to 'atan' (number expected, got string)");
                       }
                   },
                   [ctx](String x, Nil /*unused*/) {
                       Vallist list = Vallist({Value(std::move(x))});
                       CallContext new_ctx = ctx.make_new(list);
                       auto res = to_number(new_ctx);

                       if (res != Nil()) {
                           auto num = get<Number>(res);

                           return Value(std::atan2(num.value, 1));
                       } else {
                           throw std::runtime_error(
                               "bad argument #1 to 'atan' (number expected, got string)");
                       }
                   },
                   [ctx](String x, auto y) -> Value {
                       Vallist list = Vallist({Value(std::move(x))});
                       CallContext new_ctx = ctx.make_new(list);
                       auto res = to_number(new_ctx);

                       if (res != Nil()) {
                           throw std::runtime_error(
                               "bad argument #2 to 'atan' (number expected, got " +
                               std::string(y.TYPE) + ")");
                       } else {
                           throw std::runtime_error(
                               "bad argument #1 to 'atan' (number expected, got string)");
                       }
                   },
                   [](auto x, auto /*unused*/) -> Value {
                       throw std::runtime_error(
                           "bad argument #1 to 'atan' (number expected, got " +
                           std::string(x.TYPE) + ")");
                   }},
               x.raw(), y.raw())
        .with_origin(origin);
}

auto ceil(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(Nil()); // TODO: add real return value
        }});

    return math_helper(ctx, static_cast<double (*)(double)>(&std::ceil), "ceil");
}

auto cos(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(std::acos(n.value));
        }});

    return math_helper(ctx, static_cast<double (*)(double)>(&std::cos), "cos").with_origin(origin);
}

auto deg(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(n * PI / 180);
        }});

    return math_helper(
               ctx, [](double d) { return d * 180 / PI; }, "deg")
        .with_origin(origin);
}

auto exp(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(std::log(n.value));
        }});
    return math_helper(ctx, static_cast<double (*)(double)>(&std::exp), "exp").with_origin(origin);
}

auto floor(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(Nil()); // TODO: add real return value
        }});

    return math_helper(ctx, static_cast<double (*)(double)>(&std::floor), "floor");
}

auto fmod(const CallContext& ctx) -> Value {
    auto origin = Origin(BinaryOrigin{
        .lhs = make_owning<Value>(ctx.arguments().get(0)),
        .rhs = make_owning<Value>(ctx.arguments().get(1)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value, const Value& old_value1,
                      const Value& old_value2) -> std::optional<SourceChangeTree> {
            return old_value1.force(Nil());
        }}); // TODO: return it correctly
    // lua throws an error if x and y are 0 so i cant use the helper-function
    Vallist list = Vallist({ctx.arguments().get(0)});
    CallContext new_ctx = ctx.make_new(list);
    auto res1 = to_number(new_ctx);

    if (res1 != Nil()) {
        list = Vallist({ctx.arguments().get(1)});
        new_ctx = ctx.make_new(list);
        auto res2 = to_number(new_ctx);

        if (res2 != Nil()) {
            auto num1 = std::get<Number>(res1);
            auto num2 = std::get<Number>(res2);

            if (num1.value == 0 && num2.value == 0) {
                // case 0/0
                throw std::runtime_error("bad argument #2 to 'fmod' (zero)");
            } else {
                return Value(std::fmod(num1.value, num2.value)).with_origin(origin);
            }
        } else {
            auto y = ctx.arguments().get(1);
            throw std::runtime_error(
                "bad argument #2 to 'fmod' (number expected, got " + y.type() + ")");
        }
    } else {
        auto x = ctx.arguments().get(0);
        throw std::runtime_error(
            "bad argument #1 to 'fmod' (number expected, got " + x.type() + ")");
    }
}

auto log(const CallContext& ctx) -> Value {
    auto origin = Origin(BinaryOrigin{
        .lhs = make_owning<Value>(ctx.arguments().get(0)),
        .rhs = make_owning<Value>(ctx.arguments().get(1)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value, const Value& old_value1,
                      const Value& old_value2) -> std::optional<SourceChangeTree> {
            return old_value1.force(Nil());
        } // TODO: return it correctly
    });

    auto x = ctx.arguments().get(0);
    auto base = ctx.arguments().get(1);

    return std::visit(
               overloaded{
                   [ctx](auto /*unused*/, Nil /*unused*/) -> Value {
                       return math_helper(ctx, static_cast<double (*)(double)>(&std::log), "log");
                   },
                   [ctx](auto a, auto b) -> Value {
                       Vallist list({Value(a)});
                       CallContext new_ctx = ctx.make_new(list);
                       auto x =
                           math_helper(new_ctx, static_cast<double (*)(double)>(&std::log), "log");
                       list = Vallist({Value(b)});
                       new_ctx = ctx.make_new(list);
                       auto base = math_helper(
                           new_ctx, static_cast<double (*)(double)>(&std::log), "log", "2");
                       Number x_n = std::get<Number>(x);
                       Number base_n = std::get<Number>(base);
                       return Value(x_n.value / base_n.value);
                   }},
               x.raw(), base.raw())
        .with_origin(origin);
}

auto max(const CallContext& ctx) -> Value {
    auto args = ctx.arguments();

    Value max = *args.begin();
    for (const auto& a : args) {
        if (a.greater_than(max)) {
            max = a;
        }
    }
    return max;
}

auto min(const CallContext& ctx) -> Value {
    auto args = ctx.arguments();

    Value min = *args.begin();
    for (const auto& a : args) {
        if (a.less_than(min)) {
            min = a;
        }
    }
    return min;
}

auto modf(const CallContext& ctx) -> Vallist {
    auto res = to_number(ctx);

    if (res != Nil()) {
        Number num = std::get<Number>(res);
        double iptr;
        num.value = std::modf(num.value, &iptr);

        auto origin1 = Origin(UnaryOrigin{
            .val = make_owning<Value>(Value(iptr)),
        });
        auto origin2 = Origin(UnaryOrigin{
            .val = make_owning<Value>(num),
        });
        return Vallist({Value(iptr).with_origin(origin1), Value(num).with_origin(origin2)});
    } else {
        auto x = ctx.arguments().get(0);
        throw std::runtime_error(
            "bad argument #1 to 'modf' (number expected, got " + x.type() + ")");
    }
}

auto rad(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(n * 180 / PI);
        }});

    return math_helper(
               ctx, [](double d) { return d * PI / 180; }, "rad")
        .with_origin(origin);
}

auto random(const CallContext& ctx) -> Value {
    auto origin = Origin(BinaryOrigin{
        .lhs = make_owning<Value>(ctx.arguments().get(0)),
        .rhs = make_owning<Value>(ctx.arguments().get(1)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value, const Value& old_value1,
                      const Value& old_value2) -> std::optional<SourceChangeTree> {
            return old_value1.force(Nil());
        } // TODO: return it correctly
    });
    auto x = ctx.arguments().get(0);
    auto y = ctx.arguments().get(1);

    return std::visit(
               overloaded{
                   [](Nil /*unused*/, Nil /*unused*/) {
                       return Value(std::uniform_real_distribution<double>(0, 1)(random_seed));
                   },
                   [ctx](auto /*unused*/, Nil /*unused*/) {
                       return math_helper(
                           ctx,
                           [](long x) {
                               return std::uniform_int_distribution<int>(1, x)(random_seed);
                           },
                           "random");
                   },
                   [ctx](auto /*unused*/, auto /*unused*/) -> Value {
                       return math_helper<int>(
                           ctx,
                           [](long x, long y) {
                               if (x <= y) {
                                   return std::uniform_int_distribution<int>(x, y)(random_seed);
                               } else {
                                   throw std::runtime_error(
                                       "bad argument #1 to 'random' (interval is empty)");
                               }
                           },
                           "random");
                   }},
               x.raw(), y.raw())
        .with_origin(origin);
}

void randomseed(const CallContext& ctx) {
    auto x = ctx.arguments().get(0);

    x = to_number(ctx);
    if (x != Nil()) {
        Number num = std::get<Number>(x);

        random_seed = std::default_random_engine((unsigned int)num.value);
    } else {
        throw std::runtime_error(
            "bad argument #1 to 'randomseed' (number expected, got " +
            ctx.arguments().get(0).type() + ")");
    }
}

auto sin(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(std::asin(n.value));
        }});
    return math_helper(ctx, static_cast<double (*)(double)>(&std::sin), "sin").with_origin(origin);
}

auto sqrt(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(n.value * n.value);
        }});
    return math_helper(ctx, static_cast<double (*)(double)>(&std::sqrt), "sqrt")
        .with_origin(origin);
}

auto tan(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(std::atan(n.value));
        }});
    return math_helper(ctx, static_cast<double (*)(double)>(&std::tan), "tan").with_origin(origin);
}

auto to_integer(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(Nil()); // TODO: add real return value
        }});

    auto x = ctx.arguments().get(0);

    return std::visit(
               overloaded{
                   [](Number n) -> Value {
                       if (std::modf(n.value, &n.value) == 0) {
                           return Value(n.value);
                       } else {
                           return Nil();
                       }
                   },
                   [](const String& s) -> Value {
                       std::regex pattern_number(R"(\s*-?\d+)");
                       std::regex pattern_hex(R"(\s*-?0[xX][\dA-Fa-f]+)");
                       std::regex pattern_exp(R"(\s*-?\d+\.?\d*[eE]\d+)");

                       if (std::regex_match(s.value, pattern_number) ||
                           std::regex_match(s.value, pattern_hex) ||
                           std::regex_match(s.value, pattern_exp)) {
                           return Value(std::stoi(s.value, nullptr, 0));
                       } else {
                           return Nil();
                       }
                   },
                   [](auto /*unused*/) -> Value { return Nil(); }},
               x.raw())
        .with_origin(origin);
}

auto type(const CallContext& ctx) -> Value {
    auto origin = Origin(UnaryOrigin{
        .val = make_owning<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            Number n = std::get<Number>(new_value);
            return old_value.force(Nil()); // TODO: add real return value
        }});

    auto x = ctx.arguments().get(0);

    if (x.is_number()) {
        Number num = std::get<Number>(x);
        double fraction = std::modf(num.value, &num.value);

        return Value(fraction == 0.0 ? "integer" : "float").with_origin(origin);
    } else {
        return Value(Nil()).with_origin(origin);
    }
}

auto ult(const CallContext& ctx) -> Value {
    auto origin = Origin(BinaryOrigin{
        .lhs = make_owning<Value>(ctx.arguments().get(0)),
        .rhs = make_owning<Value>(ctx.arguments().get(1)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value, const Value& old_value1,
                      const Value& old_value2) -> std::optional<SourceChangeTree> {
            return old_value1.force(Nil());
        }}); // TODO: return it correctly

    return math_helper<bool>(
               ctx,
               [](double m, double n) -> bool {
                   double num_m;
                   double fraction = std::modf(m, &num_m);

                   if (fraction == 0.0) {
                       double num_n;
                       fraction = std::modf(n, &num_n);
                       if (fraction == 0.0) {
                           unsigned long ml = (long)num_m;
                           unsigned long nl = (long)n;
                           return ml < nl;
                       } else {
                           throw std::runtime_error(
                               "bad argument #2 to 'ult' (number has no integer representation)");
                       }
                   } else {
                       throw std::runtime_error(
                           "bad argument #1 to 'ult' (number has no integer representation)");
                   }
               },
               "ult")
        .with_origin(origin);
}

auto get_random_seed() -> std::default_random_engine { return random_seed; }
} // namespace minilua::math