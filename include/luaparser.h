#ifndef LUAPARSER_H
#define LUAPARSER_H

#include "luaast.h"

#include <string>
#include <variant>
#include <regex>
#include <vector>
#include <utility>
#include <memory>
#include <unordered_map>

using namespace std;

class LuaParser {
public:
    LuaParser();

    template <typename T = LuaAST>
    using parse_result_t = variant<T, string>;

    using token_list_t = vector<LuaToken>;
    using token_it_t = token_list_t::iterator;

    auto parse(const string& program) -> parse_result_t<LuaChunk>;

private:
    const static vector<pair<regex, LuaToken::Type>> token_regexes;

    auto tokenize(string::const_iterator begin, string::const_iterator end) -> token_list_t;

    auto parse_chunk            (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaChunk>;
    auto parse_block            (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaChunk>;
    auto parse_stat             (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaStmt>;
    auto parse_laststat         (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaStmt>;
    auto parse_funcname         (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaVar>;
    auto parse_varlist          (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist>;
    auto parse_var              (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaVar>;
    auto parse_namelist         (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist>;
    auto parse_explist          (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist>;
    auto parse_exp              (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExp>;
    auto parse_prefixexp        (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExp>;
    auto parse_functioncall     (token_it_t& begin, token_it_t& end, const LuaExp& prefixexp) const -> parse_result_t<LuaFunctioncall>;
    auto parse_args             (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist>;
    auto parse_function         (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaFunction>;
    auto parse_funcbody         (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaFunction>;
    auto parse_parlist          (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist>;
    auto parse_tableconstructor (token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaTableconstructor>;
    auto parse_fieldlist        (token_it_t& begin, token_it_t& end) const -> parse_result_t<>;
    auto parse_field            (token_it_t& begin, token_it_t& end) const -> parse_result_t<>;
    auto parse_fieldsep         (token_it_t& begin, token_it_t& end) const -> parse_result_t<>;
    auto parse_binop            (token_it_t& begin, token_it_t& end) const -> parse_result_t<>;
    auto parse_unop             (token_it_t& begin, token_it_t& end) const -> parse_result_t<>;
};

#endif // LUAPARSER_H
