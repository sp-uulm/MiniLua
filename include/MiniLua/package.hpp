#ifndef MINILUA_PACKAGE
#define MINILUA_PACKAGE
#endif

#include <MiniLua/values.hpp>

namespace minilua {
auto require(const CallContext& ctx) -> Value;

namespace package {
auto searchpath(const CallContext& ctx) -> Vallist;
auto find_loader(const CallContext& ctx) -> Vallist;
auto require(const CallContext &ctx) -> Value;
} // end namespace package
} // end namespace minilua