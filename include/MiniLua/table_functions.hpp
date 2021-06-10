#ifndef MINILUA_TABLE_FUNCTIONS_HPP
#define MINILUA_TABLE_FUNCTIONS_HPP

#include <MiniLua/values.hpp>

namespace minilua {

namespace table {
auto concat(const CallContext& ctx) -> Value;

void insert(const CallContext& ctx);

auto pack(const CallContext& ctx) -> Value;

auto unpack(const CallContext& ctx) -> Vallist;
} // end namespace table
} // end namespace minilua
#endif