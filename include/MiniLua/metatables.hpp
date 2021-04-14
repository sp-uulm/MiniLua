#ifndef MINILUA_METATABLES_HPP
#define MINILUA_METATABLES_HPP

#include "values.hpp"

/**
 * @brief contains the *metatable events*.
 *
 * That is everything that can be customized using a metamethod.
 *
 * The functions have the same name as the metamethods (except without leading
 * `__`). These names don't always match the method names on `Value`.
 *
 * If the required metamethod is not present (or the value is not a table)
 * these functions will simply call the corresponding functions on the `Value`.
 *
 * \note This is slightly different to lua spec. In Lua it would be possible to
 * set metatables (globally) for string, number, bool, nil and function as well
 * but we don't support this (this would only be possible through the debug api
 * or C anyway).
 *
 * Binary events first check the left value for the metamethod and if that is
 * not present they check the right side.
 */
namespace minilua::mt {

// able access operators

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

// binary operators

/**
 * @brief Add (binary `+`) operator. Will call metamethod `__add` if necessary.
 */
auto add(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Sub (binary `-`) operator. Will call metamethod `__sub` if necessary.
 */
auto sub(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Mul (binary `*`) operator. Will call metamethod `__mul` if necessary.
 */
auto mul(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Div (binary `/`) operator. Will call metamethod `__div` if necessary.
 */
auto div(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Mod (binary `%`) operator. Will call metamethod `__mod` if necessary.
 */
auto mod(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Pow (binary `^`) operator. Will call metamethod `__pow` if necessary.
 */
auto pow(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Floor division (binary `//`) operator. Will call metamethod `__idiv` if necessary.
 */
auto idiv(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Bitwise and (binary `&`) operator. Will call metamethod `__band` if necessary.
 */
auto band(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Bitwise or (binary `|`) operator. Will call metamethod `__bor` if necessary.
 */
auto bor(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Bitwise xor (binary `~`) operator. Will call metamethod `__bxor` if necessary.
 */
auto bxor(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Bitwise left shift (binary `<<`) operator. Will call metamethod `__shl` if necessary.
 */
auto shl(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Bitwise right shift (binary `>>`) operator. Will call metamethod `__shr` if necessary.
 */
auto shr(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Concatenation (binary `..`) operator. Will call metamethod `__concat` if necessary.
 */
auto concat(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Equal (binary `==`) operator. Will call metamethod `__eq` if necessary.
 *
 * Lua will only try the metamethods if both values are the same type and not
 * trivially equal.
 *
 * The result is always converted to a bool.
 */
auto eq(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Less than (binary `<`) operator. Will call metamethod `__lt` if necessary.
 *
 * The result is always converted to a bool.
 */
auto lt(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Less than (binary `<=`) operator. Will call metamethod `__le` if necessary.
 *
 * The lua spec also specifies that if the `__le` metamethod is not present it
 * will try a `__lt` metamethod with reversed parameters. But it states that
 * this behaviour might be removed in the future (this assumes that `a <= b` is
 * equal to `b < a`).
 *
 * The result is always converted to a bool.
 */
auto le(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

// unary operators

/**
 * @brief Negation (unary `-`) operator. Will call metamethod `__unm` if necessary.
 */
auto unm(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Bitwise not (unary `~`) operator. Will call metamethod `__bnot` if necessary.
 */
auto bnot(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

/**
 * @brief Length (unary `#`) operator. Will call metamethod `__len` if necessary.
 */
auto len(const CallContext& ctx, std::optional<Range> location = std::nullopt) -> CallResult;

// call operator

/**
 * @brief Call (`func(args)`) operator. Will call metamethod `__call` if necessary.
 *
 * The metamethod is only searched in `func` and it will get `func` and the
 * arguments `args` as parameters.
 *
 * This metamethod allows multiple results.
 */
auto call(const CallContext& ctx) -> CallResult;

/**
 * @brief Called when a table is *garbage collected*. Will call metamethod `__gc`.
 *
 * In our case the tables are all *garbage collected* at once when the lua
 * program stops running.
 *
 * \note The return value and source changes of `__gc` are ignored.
 */
void gc(const CallContext& ctx);

} // namespace minilua::mt

#endif
