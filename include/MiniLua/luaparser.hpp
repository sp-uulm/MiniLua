#ifndef LUAPARSER_H
#define LUAPARSER_H

#include "luaast.hpp"

#include <boost/spirit/include/lex_lexertl.hpp>
#include <chrono>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace std;
namespace bs = boost::spirit;

constexpr size_t WS = 1000;

template <typename Lexer> struct lua_tokens : bs::lex::lexer<Lexer> {
    lua_tokens() {
        // define tokens (the regular expression to match and the corresponding
        // token id) and add them to the lexer
        this->self.add("\\s+", WS)("--\\[\\[[^]*?\\]\\]", LuaToken::Type::BLOCKCOMMENT)("--[^\n]*", LuaToken::Type::COMMENT)
                ("(\\\"[^\\\"]*\\\")|('[^']*')", LuaToken::Type::STRINGLIT)
                ("((\\d+\\.?\\d*)|(\\d*\\.?\\d+))(e-?\\d+)?", LuaToken::Type::NUMLIT)("\\+", LuaToken::Type::ADD)(
            "-", LuaToken::Type::SUB)("\\*", LuaToken::Type::MUL)("\\/", LuaToken::Type::DIV)(
            "%", LuaToken::Type::MOD)("\\^", LuaToken::Type::POW)("\\#", LuaToken::Type::LEN)(
            "\\$", LuaToken::Type::STRIP)("\\\\", LuaToken::Type::EVAL)("==", LuaToken::Type::EQ)(
            "~=", LuaToken::Type::NEQ)("<=", LuaToken::Type::LEQ)(">=", LuaToken::Type::GEQ)(
            "<", LuaToken::Type::LT)(">", LuaToken::Type::GT)("=", LuaToken::Type::ASSIGN)(
            "\\{", LuaToken::Type::LCB)("\\}", LuaToken::Type::RCB)("\\(", LuaToken::Type::LRB)(
            "\\)", LuaToken::Type::RRB)("\\[", LuaToken::Type::LSB)("\\]", LuaToken::Type::RSB)(
            ";", LuaToken::Type::SEM)(":", LuaToken::Type::COLON)(",", LuaToken::Type::COMMA)(
            "\\.\\.\\.", LuaToken::Type::ELLIPSE)("\\.\\.", LuaToken::Type::CONCAT)(
            "\\.", LuaToken::Type::DOT)("and\\W", /* \b is not supported :-(*/ LuaToken::Type::AND)(
            "break\\W", LuaToken::Type::BREAK)("do\\W", LuaToken::Type::DO)(
            "elseif\\W", LuaToken::Type::ELSEIF)("else\\W", LuaToken::Type::ELSE)(
            "end", LuaToken::Type::END)("false", LuaToken::Type::FALSE)(
            "for\\W", LuaToken::Type::FOR)("function\\W", LuaToken::Type::FUNCTION)(
            "if\\W", LuaToken::Type::IF)("in\\W", LuaToken::Type::IN)("local\\W",
                                                                      LuaToken::Type::LOCAL)(
            "nil", LuaToken::Type::NIL)("not\\W", LuaToken::Type::NOT)("or\\W", LuaToken::Type::OR)(
            "repeat\\W", LuaToken::Type::REPEAT)("return\\W", LuaToken::Type::RETURN)(
            "then\\W", LuaToken::Type::THEN)("true", LuaToken::Type::TRUE)("until\\W",
                                                                           LuaToken::Type::UNTIL)(
            "while\\W", LuaToken::Type::WHILE)("[a-zA-Z_]\\w*", LuaToken::Type::NAME);
    }
};

struct PerformanceStatistics {
    chrono::microseconds parse;
    chrono::microseconds execute;
    chrono::microseconds tokenize;
    chrono::microseconds source_changes;
    chrono::microseconds marker_interface;
    chrono::microseconds create_env;
    chrono::microseconds total;
};

class LuaParser {
public:
    LuaParser();

    template <typename T = LuaAST> using parse_result_t = variant<T, string>;

    using token_list_t = vector<LuaToken>;
    using token_it_t = token_list_t::const_iterator;

    auto parse(const string program, PerformanceStatistics& ps) -> parse_result_t<LuaChunk>;

    token_list_t tokens;

private:
    using token_type =
        bs::lex::lexertl::token<string::const_iterator, boost::mpl::vector0<>, boost::mpl::false_>;
    using lexer_type = bs::lex::lexertl::lexer<token_type>;

    lua_tokens<lexer_type> lua_lexer;

    auto tokenize(string::const_iterator begin, string::const_iterator end) -> token_list_t;

    auto parse_chunk(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaChunk>;
    auto parse_block(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaChunk>;
    auto parse_stat(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaStmt>;
    auto parse_laststat(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaStmt>;
    auto parse_funcname(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaVar>;
    auto parse_varlist(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist>;
    auto parse_var(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaVar>;
    auto parse_namelist(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist>;
    auto parse_explist(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist>;
    auto parse_exp(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExp>;
    auto parse_prefixexp(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExp>;
    auto parse_functioncall(token_it_t& begin, token_it_t& end, const LuaExp& prefixexp) const
        -> parse_result_t<LuaFunctioncall>;
    auto parse_args(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist>;
    auto parse_function(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaFunction>;
    auto parse_funcbody(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaFunction>;
    auto parse_parlist(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist>;
    auto parse_tableconstructor(token_it_t& begin, token_it_t& end) const
        -> parse_result_t<LuaTableconstructor>;
    auto parse_field(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaField>;
    auto lua_token_to_string(LuaToken::Type type) const -> std::string;
};

string get_string(const LuaParser::token_list_t& tokens);

#endif // LUAPARSER_H
