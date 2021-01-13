#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

#include "MiniLua/math.hpp"
#include "MiniLua/stdlib.hpp"
#include "MiniLua/utils.hpp"
#include "MiniLua/values.hpp"

namespace minilua::math {

// All math-functions can be called be a number or a string that is formated like a number.
// This information is not documented in the documentation. I just found that when I was testing the
// erro behaviour for wrong input of the function in lua That is the reason why everywhere to_number
// is called.

auto abs(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);

        return Value(std::abs(num.value));
    } else {
        throw std::runtime_error("bad argument #1 to abs (number expected, got " + x.type() + ")");
    }
}

auto acos(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);

        if (num.value >= -1 && num.value <= 1) {
            return Value(std::acos(num.value));
        } else {
            return Value("nan");
        }
    } else {
        throw std::runtime_error("bad argument #1 to acos (number expected, got " + x.type() + ")");
    }
}

auto asin(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);

        if (num.value >= -1 && num.value <= 1) {
            return Value(std::asin(num.value));
        } else {
            return Value("nan");
        }
    } else {
        throw std::runtime_error("bad argument #1 to asin (number expected, got " + x.type() + ")");
    }
}

auto atan(const CallContext& ctx) -> Value {
    // unimplemented
}

auto ceil(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);

        return Value(std::ceil(num.value));
    } else {
        throw std::runtime_error("bad argument #1 to ceil (number expected, got " + x.type() + ")");
    }
}

auto cos(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);

        return Value(std::cos(num.value));
    } else {
        throw std::runtime_error("bad argument #1 to cos (number expected, got " + x.type() + ")");
    }
}

auto deg(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);
        double deg = num.value * 180 / boost::math::constants::pi<double>();

        return Value(deg);
    } else {
        throw std::runtime_error("bad argument #1 to deg (number expected, got " + x.type() + ")");
    }
}

auto exp(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);

        return Value(std::exp(num.value));
    } else {
        throw std::runtime_error("bad argument #1 to exp (number expected, got " + x.type() + ")");
    }
}
} // namespace minilua::math