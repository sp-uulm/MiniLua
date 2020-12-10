#ifndef STDLIB_HPP
#define STDLIB_HPP

#include <string>
#include <utility>

#include "MiniLua/values.hpp"

namespace minilua{
static auto split_string(std::string s, char c) -> std::pair<std::string, std::string>;

auto to_string(const CallContext& ctx) -> Value;

auto to_number(const CallContext& ctx) -> Value;

auto type(const CallContext& ctx) -> Value;
}

#endif