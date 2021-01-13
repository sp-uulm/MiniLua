#ifndef MINILUA_MATH_HPP
#define MINILUA_MATH_HPP

#include "boost/math/constants/constants.hpp"

#include "MiniLua/values.hpp"

namespace minilua::math {
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