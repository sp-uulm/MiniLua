#ifndef MINILUA_TABLE_FUNCTIONS_HPP
#define MINILUA_TABLE_FUNCTIONS_HPP

#include <MiniLua/values.hpp>

namespace minilua::table {

auto concat(const CallContext& ctx) -> Value;

void insert(const CallContext& ctx);

auto move(const CallContext& ctx) -> Value;

auto pack(const CallContext& ctx) -> Value;

auto remove(const CallContext& ctx) -> Value;

void sort(const CallContext& ctx);

auto unpack(const CallContext& ctx) -> Vallist;
} // end namespace minilua::table
#endif