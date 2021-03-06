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
auto index(const CallContext& ctx) -> CallResult;

/**
 * @brief Index into a Table or calls `__index` of the metatable.
 *
 * \note The argument has to be a CallContext because we might need to call
 * a lua function.
 */
auto newindex(const CallContext& ctx) -> CallResult;

/**
 * @brief Add (`+`) operator. Will call metamethod `__add` if necessary.
 *
 * First checks if the left side has the metamethod then the right side.
 */
auto add(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/*
__sub: the subtraction (-) operation. Behavior similar to the addition operation.
__mul: the multiplication (*) operation. Behavior similar to the addition operation.
__div: the division (/) operation. Behavior similar to the addition operation.
__mod: the modulo (%) operation. Behavior similar to the addition operation.
__pow: the exponentiation (^) operation. Behavior similar to the addition operation.
__unm: the negation (unary -) operation. Behavior similar to the addition operation.
__idiv: the floor division (//) operation. Behavior similar to the addition operation.
__band: the bitwise AND (&) operation. Behavior similar to the addition operation, except that Lua
will try a metamethod if any operand is neither an integer nor a value coercible to an integer (see
§3.4.3).
__bor: the bitwise OR (|) operation. Behavior similar to the bitwise AND operation.
__bxor: the bitwise exclusive OR (binary ~) operation. Behavior similar to the bitwise AND
operation.
__bnot: the bitwise NOT (unary ~) operation. Behavior similar to the bitwise AND operation.
__shl: the bitwise left shift (<<) operation. Behavior similar to the bitwise AND operation.
__shr: the bitwise right shift (>>) operation. Behavior similar to the bitwise AND operation.
__concat: the concatenation (..) operation. Behavior similar to the addition operation, except that
Lua will try a metamethod if any operand is neither a string nor a number (which is always coercible
to a string).
__len: the length (#) operation. If the object is not a string, Lua will try its metamethod. If
there is a metamethod, Lua calls it with the object as argument, and the result of the call (always
adjusted to one value) is the result of the operation. If there is no metamethod but the object is a
table, then Lua uses the table length operation (see §3.4.7). Otherwise, Lua raises an error.
__eq: the equal (==) operation. Behavior similar to the addition operation, except that Lua will try
a metamethod only when the values being compared are either both tables or both full userdata and
they are not primitively equal. The result of the call is always converted to a boolean.
__lt: the less than (<) operation. Behavior similar to the addition operation, except that Lua will
try a metamethod only when the values being compared are neither both numbers nor both strings. The
result of the call is always converted to a boolean.
__le: the less equal (<=) operation. Unlike other operations, the less-equal operation can use two
different events. First, Lua looks for the __le metamethod in both operands, like in the less than
operation. If it cannot find such a metamethod, then it will try the __lt metamethod, assuming that
a <= b is equivalent to not (b < a). As with the other comparison operators, the result is always a
boolean. (This use of the __lt event can be removed in future versions; it is also slower than a
real __le metamethod.)
__call: The call operation func(args). This event happens when Lua tries to call a non-function
value (that is, func is not a function). The metamethod is looked up in func. If present, the
metamethod is called with func as its first argument, followed by the arguments of the original call
(args). All results of the call are the result of the operation. (This is the only metamethod that
allows multiple results.)
*/

} // namespace minilua::mt

#endif
