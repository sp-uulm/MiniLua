#ifndef OPERATORS_H
#define OPERATORS_H

#include "luatoken.hpp"
#include "val.hpp"

namespace lua {
namespace rt {

eval_result_t op_add(val a, val b, const LuaToken& tok = {LuaToken::Type::ADD, ""});
inline val operator+(const val& a, const val& b) { return unwrap(op_add(a, b)); }

eval_result_t op_sub(val a, val b, const LuaToken& tok = {LuaToken::Type::SUB, ""});
inline val operator-(const val& a, const val& b) { return unwrap(op_sub(a, b)); }

eval_result_t op_mul(val a, val b, const LuaToken& tok = {LuaToken::Type::MUL, ""});
inline val operator*(const val& a, const val& b) { return unwrap(op_mul(a, b)); }

eval_result_t op_div(val a, val b, const LuaToken& tok = {LuaToken::Type::DIV, ""});
inline val operator/(const val& a, const val& b) { return unwrap(op_div(a, b)); }

eval_result_t op_pow(val a, val b, const LuaToken& tok = {LuaToken::Type::POW, ""});
inline val operator^(const val& a, const val& b) { return unwrap(op_pow(a, b)); }

eval_result_t op_mod(val a, val b, const LuaToken& tok = {LuaToken::Type::MOD, ""});
eval_result_t op_concat(val a, val b);
eval_result_t op_eval(val a, val b, const LuaToken& tok = {LuaToken::Type::EVAL, ""});
eval_result_t op_postfix_eval(val a, const LuaToken& tok = {LuaToken::Type::EVAL, ""});

eval_result_t op_lt(val a, val b);
inline bool operator<(const val& a, const val& b) { return get<bool>(unwrap(op_lt(a, b))); }

eval_result_t op_leq(val a, val b);
inline bool operator<=(const val& a, const val& b) { return get<bool>(unwrap(op_leq(a, b))); }

eval_result_t op_gt(val a, val b);
inline bool operator>(const val& a, const val& b) { return get<bool>(unwrap(op_gt(a, b))); }

eval_result_t op_geq(val a, val b);
inline bool operator>=(const val& a, const val& b) { return get<bool>(unwrap(op_geq(a, b))); }

eval_result_t op_eq(val a, val b);
inline bool operator==(const val& a, const val& b) { return get<bool>(unwrap(op_eq(a, b))); }

eval_result_t op_neq(val a, val b);
inline bool operator!=(const val& a, const val& b) { return get<bool>(unwrap(op_neq(a, b))); }

eval_result_t op_and(val a, val b);
inline val operator&&(const val& a, const val& b) { return unwrap(op_and(a, b)); }

eval_result_t op_or(val a, val b);
inline val operator||(const val& a, const val& b) { return unwrap(op_or(a, b)); }

eval_result_t op_len(val v);

eval_result_t op_not(val v);
inline bool operator!(const val& a) { return get<bool>(unwrap(op_not(a))); }

eval_result_t op_neg(val v, const LuaToken& tok = {LuaToken::Type::SUB, ""});
inline val operator-(const val& a) { return unwrap(op_neg(a)); }

eval_result_t op_sqrt(val v);
eval_result_t op_strip(val v);

} // namespace rt
} // namespace lua

#endif
