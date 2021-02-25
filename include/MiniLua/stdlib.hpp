#ifndef MINILUA_STDLIB_HPP
#define MINILUA_STDLIB_HPP

#include <string>
#include <utility>

#include "MiniLua/values.hpp"

namespace minilua {

/**
 * The same as calling `arg1.force(arg2)` where `arg1` and `arg2` are the first
 * and second argument.
 */
auto force(const CallContext& ctx) -> CallResult;

auto to_string(const CallContext& ctx) -> Value;

auto to_number(const CallContext& ctx) -> Value;

auto type(const CallContext& ctx) -> Value;

auto assert_lua(const CallContext& ctx) -> Vallist;

auto next(const CallContext& ctx) -> Vallist;

/**
 * If `index` is a number, returns all arguments after the `index`-th argument
 * (including `index`-th argument).
 *
 * If index is a negative number, the index counting starts at the end (so `-1`
 * is the index of the last argument). If index is the string `"#"`, select
 * returns the total amount of extra arguments it received. If index is
 * positive and bigger than the size select just returns an empty Vallist. If
 * index in an invalid index (0 or negative bigger than size) an index out of
 * range error is thrown if index is an invalid value, an error is thrown that
 * an invalid index was entered.
 */
auto select(const CallContext& ctx) -> Vallist;

void print(const CallContext& ctx);

/**
 * Remove the origin of all passed in values and return them.
 *
 * \note This is not part of the official lua standard.
 */
auto discard_origin(const CallContext& ctx) -> Vallist;

} // namespace minilua

#endif
