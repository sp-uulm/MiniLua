#ifndef MINILUA_MATH_HPP
#define MINILUA_MATH_HPP

#include <climits>
#include <math.h>

#include "MiniLua/values.hpp"

const double PI = M_PI;

namespace minilua::math {
const long MAXINTEGER = LONG_MAX;
const long MININTEGER = LONG_MIN;

auto abs(const CallContext& ctx) -> Value;

auto acos(const CallContext& ctx) -> Value;

auto asin(const CallContext& ctx) -> Value;

auto atan(const CallContext& ctx) -> Value;

auto ceil(const CallContext& ctx) -> Value;

auto cos(const CallContext& ctx) -> Value;

auto deg(const CallContext& ctx) -> Value;

auto exp(const CallContext& ctx) -> Value;
} // namespace minilua::math

#endif