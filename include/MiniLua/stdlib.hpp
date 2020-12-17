#ifndef STDLIB_HPP
#define STDLIB_HPP

#include <string>
#include <utility>

#include "MiniLua/values.hpp"

namespace minilua {
auto to_string(const CallContext& ctx) -> Value;

auto to_number(const CallContext& ctx) -> Value;

auto type(const CallContext& ctx) -> Value;

auto assert(const CallContext& ctx) -> Value;
} // namespace minilua

#endif