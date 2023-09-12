#ifndef MINILUA_STRING
#define MINILUA_STRING

#include <MiniLua/values.hpp>

namespace minilua::string {
auto byte(const CallContext& ctx) -> Vallist;
auto lua_char(const CallContext& ctx) -> Value;
// auto dump(const CallContext& ctx) -> Value; //probably can be scraped
// auto find(const CallContext& ctx) -> Vallist;
auto format(const CallContext& ctx) -> Value;
// auto gmatch(const CallContext& ctx) -> Value;
// auto gsub(const CallContext& ctx) -> Vallist;
auto len(const CallContext& ctx) -> Value;
auto lower(const CallContext& ctx) -> Value;
// auto match(const CallContext& ctx) -> Value;
// auto pack(const CallContext& ctx) -> Value;
// auto packsize(const CallContext& ctx) -> Value;
auto rep(const CallContext& ctx) -> Value;
auto reverse(const CallContext& ctx) -> Value;
auto sub(const CallContext& ctx) -> Value;
// auto unpack(const CallContext& ctx) -> Vallist;
auto upper(const CallContext& ctx) -> Value;
} // namespace minilua::string
#endif