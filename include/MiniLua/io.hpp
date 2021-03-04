#ifndef MINILUA_IO_HPP
#define MINILUA_IO_HPP

#include "MiniLua/values.hpp"

namespace minilua::io {

auto open(const CallContext& ctx) -> Vallist;
} // namespace minilua::io

#endif