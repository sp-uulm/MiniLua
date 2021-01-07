#ifndef MINILUA_STDLIB_HPP
#define MINILUA_STDLIB_HPP

#include <string>
#include <utility>

#include "MiniLua/values.hpp"

namespace minilua {
auto to_string(const CallContext& ctx) -> Value;

auto to_number(const CallContext& ctx) -> Value;

auto type(const CallContext& ctx) -> Value;

auto assert_lua(const CallContext& ctx) -> Vallist;

auto next(const CallContext& ctx) -> Vallist;

auto select(const CallContext& ctx) -> Vallist;
} // namespace minilua

#endif