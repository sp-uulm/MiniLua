#ifndef MINILUA_METATABLES_HPP
#define MINILUA_METATABLES_HPP

#include "values.hpp"

namespace minilua::mt {

/**
 * @brief Index into a Table or calls `__index` of the metatable.
 *
 * The metatable is only used if the key is not present in the table.
 *
 * \note The argument has to be a CallContext because we might need to call
 * a lua function.
 */
[[nodiscard]] auto index(const CallContext& ctx) -> CallResult;

/**
 * @brief Index into a Table or calls `__index` of the metatable.
 *
 * \note The argument has to be a CallContext because we might need to call
 * a lua function.
 */
[[nodiscard]] auto newindex(const CallContext& ctx) -> CallResult;

} // namespace minilua::mt

#endif
