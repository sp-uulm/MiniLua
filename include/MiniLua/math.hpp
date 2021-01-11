#ifndef MINILUA_MATH_HPP
#define MINILUA_MATH_HPP

#include "MiniLua/values.hpp"

namespace minilua::math {
auto abs(const CallContext& ctx) -> Value;
} // namespace minilua::math

#endif