#include <cmath>
#include <iostream>
#include <optional>
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
static const std::string pattern_number(R"(\s*-?\d+)");
static const std::string pattern_hex(R"(\s*-?0[xX][\dA-Fa-f]+)");
static const std::string pattern_exp(R"(\s*-?\d+\.?\d*[eE]\d+)");
static const std::regex
    pattern_all_number_variants(pattern_number + "|" + pattern_hex + "|" + pattern_exp);

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
            if (n.value >= 0) {
                return old_value.force(n); // just guess that it was a positive number
            } else {
                return std::nullopt;
            }
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
    double len = -1;
    auto origin = Origin(BinaryOrigin{
        .lhs = make_owning<Value>(ctx.arguments().get(0)),
        .rhs = make_owning<Value>(ctx.arguments().get(1)),
        .location = ctx.call_location(),
        .reverse = [len](const Value& new_value, const Value& old_value1, const Value& old_value2)
            -> std::optional<SourceChangeTree> {
            if (old_value2 == Nil()) {
                Number n = std::get<Number>(new_value);
                return old_value1.force(std::tan(n.value));
            } else {
                double theta = std::get<Number>(new_value).value;
                old_value1.force(len * std::sin(theta));
                return old_value2.force(len * std::cos(theta));
            }
        }});

    auto x = ctx.arguments().get(0);
    auto y = ctx.arguments().get(1);

    return std::visit(
               overloaded{
                   [&len](Number x, Number y) {
                       len = std::sqrt(x.value * x.value + y.value * y.value);
                       return Value(std::atan2(x.value, y.value));
                   },
                   [ctx, &len](Number x, String y) {
                       Vallist list = Vallist({Value(std::move(y))});
                       CallContext new_ctx = ctx.make_new(list);
                       auto res = to_number(new_ctx);

                       if (res != Nil()) {
                           auto num = get<Number>(res);

                           len = std::sqrt(x.value * x.value + num.value * num.value);
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
                   [ctx, &len](String x, Number y) {
                       Vallist list = Vallist({Value(std::move(x))});
                       CallContext new_ctx = ctx.make_new(list);
                       auto res = to_number(new_ctx);

                       if (res != Nil()) {
                           auto num = get<Number>(res);

                           len = std::sqrt(num.value * num.value + y.value * y.value);
                           return Value(std::atan2(num.value, y.value));
                       } else {
                           throw std::runtime_error(
                               "bad argument #1 to 'atan' (number expected, got string)");
                       }
                   },
                   [ctx, &len](String x, String y) {
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

                               len = std::sqrt(x1.value * x1.value + x2.value * x2.value);
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
            if (std::fmod(n.value, 1.0) == 0.0) {
                return old_value.force(
                    new_value); // Just guessing that this is the right value because every
                                // information about post-comma numbers is lost
            } else {
                return std::nullopt;
            }
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
            if (n > 0) {
                return old_value.force(std::log(n.value));
            } else if (n < 0) {
                return old_value.force(1 / std::log(n.value));
            } else {
                return std::nullopt;
            }
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
            if (std::fmod(n.value, 1.0) == 0.0) {
                return old_value.force(new_value);
                // Just guessing that this is the right value because every information about
                // post-comma numbers is lost
            } else {
                return std::nullopt;
            }
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
            if (old_value2 == Nil()) {
                Number n = std::get<Number>(new_value);
                return old_value1.force(std::exp(n.value));
            } else {
                // TODO: return both SourceChangeAlternatives
                // log(x)/log(base) is ambiguously. so no clear return could be done
                // if someone knows a way feel free to change it.
                return std::nullopt;
            }
        }});

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
    Origin origin = Origin(MultipleArgsOrigin{
        .values = ctx.arguments(),
        .location = ctx.call_location(),
        .reverse = [](const Value& /*new_value*/,
                      const Vallist& /*args*/) -> std::optional<SourceChangeTree> {
            return std::nullopt; // TODO: insert correct reverse (by calling max again?)
        }});
    auto args = ctx.arguments();

    if (args.size() == 0) {
        throw std::runtime_error("bad argument #1 to 'max' (value expected)");
    }

    Value max = *args.begin();
    for (const auto& a : args) {
        if (a.greater_than(max)) {
            max = a;
        }
    }
    return max.with_origin(origin);
}

auto min(const CallContext& ctx) -> Value {
    Origin origin = Origin(MultipleArgsOrigin{
        .values = ctx.arguments(),
        .location = ctx.call_location(),
        .reverse = [](const Value& /*new_value*/,
                      const Vallist& /*args*/) -> std::optional<SourceChangeTree> {
            return std::nullopt; // TODO: insert correct reverse (by calling max again?)
        }});
    auto args = ctx.arguments();

    if (args.size() == 0) {
        throw std::runtime_error("bad argument #1 to 'min' (value expected)");
    }

    Value min = *args.begin();
    for (const auto& a : args) {
        if (a.less_than(min)) {
            min = a;
        }
    }
    return min.with_origin(origin);
}

auto modf(const CallContext& ctx) -> Vallist {
    auto res = to_number(ctx);

    if (res != Nil()) {
        Number num = std::get<Number>(res);
        double iptr;
        num.value = std::modf(num.value, &iptr);

        auto origin1 = Origin(UnaryOrigin{
            .val = make_owning<Value>(Value(iptr)),
            .location = ctx.call_location(), // is that the correct location?
            .reverse = [](const Value& new_value,
                          const Value& old_value) -> std::optional<SourceChangeTree> {
                return std::nullopt; // TODO: add real reverse. this is only temporary
            }});
        auto origin2 = Origin(UnaryOrigin{
            .val = make_owning<Value>(num),
            .location = ctx.call_location(), // is that the correct location?
            .reverse = [](const Value& new_value,
                          const Value& old_value) -> std::optional<SourceChangeTree> {
                return std::nullopt; // TODO: add real reverse. this is only temporary
            }});
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
    // A random value can't be forced. thats why random has no origin
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
                    [](long x) { return std::uniform_int_distribution<int>(1, x)(random_seed); },
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
        x.raw(), y.raw());
}

// no origin because no value is changed.
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
            if (n.value >= 0) {
                return old_value.force(n.value * n.value);
            } else {
                return std::nullopt;
            }
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
            if (new_value == Nil() || std::fmod(n.value, 1.0) == 0.0) {
                return std::nullopt;
                // if result is Nil, converting to an integer was not possible.
                // so it isnt possible to revert anything.
            } else {
                return old_value.force(new_value);
            }
            // Second solution (i dont know which is the correct one)
            // please select the right one in the review and delete the wrong one
            // return old_value.force(new_value);
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
                       if (std::regex_match(s.value, pattern_all_number_variants)) {
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
    // only with the information of the type it is not reversable
    auto x = ctx.arguments().get(0);

    if (x.is_number()) {
        Number num = std::get<Number>(x);
        double fraction = std::modf(num.value, &num.value);

        return Value(fraction == 0.0 ? "integer" : "float");
    } else {
        return Value(Nil());
    }
}

auto ult(const CallContext& ctx) -> Value {
    // Not reverseable because only an bool is avaible and with only the bool, its not
    // possible to gain correctly the numbers back. So no origin is needed
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
        "ult");
}

auto get_random_seed() -> std::default_random_engine { return random_seed; }
} // namespace minilua::math