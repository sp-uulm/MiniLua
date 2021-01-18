#ifndef MINILUA_MATH_HPP
#define MINILUA_MATH_HPP

#include <climits>
#include <cmath>
#include <functional>
#include <string>

#include "MiniLua/values.hpp"

namespace minilua::math {
const long MAXINTEGER = LONG_MAX;
const long MININTEGER = LONG_MIN;
const double PI = M_PI;
const double HUGE = HUGE_VAL;

auto abs(const CallContext& ctx) -> Value;

auto acos(const CallContext& ctx) -> Value;

auto asin(const CallContext& ctx) -> Value;

auto atan(const CallContext& ctx) -> Value;

auto ceil(const CallContext& ctx) -> Value;

auto cos(const CallContext& ctx) -> Value;

auto deg(const CallContext& ctx) -> Value;

auto exp(const CallContext& ctx) -> Value;

auto floor(const CallContext& ctx) -> Value;

auto fmod(const CallContext& ctx) -> Value;

auto log(const CallContext& ctx) -> Value;

auto max(const CallContext& ctx) -> Value;

auto min(const CallContext& ctx) -> Value;

auto modf(const CallContext& ctx) -> Vallist;
} // namespace minilua::math

#endif