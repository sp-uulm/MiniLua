#ifndef LUAAST_H
#define LUAAST_H

#include <memory>
#include <vector>
#include <iostream>
#include <variant>
#include <unordered_map>
#include <type_traits>
#include <functional>

using namespace std;

struct LuaToken {
    enum class Type {
        NONE,
        ADD, SUB, MUL, DIV, MOD, POW, LEN, //+, -, *, /, %, ^, #
        EQ, NEQ, LEQ, GEQ, LT, GT, ASSIGN, //==, ~=, <=, >=, <, >, =
        LCB, RCB, LRB, RRB, LSB, RSB, //{, }, (, ), [, ]
        SEM, COLON, COMMA, DOT, CONCAT, ELLIPSE, //;, :, ,, ., .., ...

        AND, BREAK, DO, ELSE, ELSEIF,
        END, FALSE, FOR, FUNCTION, IF,
        IN, LOCAL, NIL, NOT, OR,
        REPEAT, RETURN, THEN, TRUE, UNTIL, WHILE,

        NAME, STRINGLIT, NUMLIT, COMMENT
    } type;
    string match;

    friend ostream& operator<<(ostream& os, const LuaToken& token);
};

using LuaAST = shared_ptr<struct _LuaAST>;
using LuaName = shared_ptr<struct _LuaName>;
using LuaExp = shared_ptr<struct _LuaExp>;
using LuaOp = shared_ptr<struct _LuaOp>;
using LuaUnop = shared_ptr<struct _LuaUnop>;
using LuaExplist = shared_ptr<struct _LuaExplist>;
using LuaFunctioncall = shared_ptr<struct _LuaFunctioncall>;
using LuaValue = shared_ptr<struct _LuaValue>;
using LuaVar = shared_ptr<struct _LuaVar>;
using LuaAssignment = shared_ptr<struct _LuaAssignment>;
using LuaNameVar = shared_ptr<struct _LuaNameVar>;
using LuaIndexVar = shared_ptr<struct _LuaIndexVar>;
using LuaMemberVar = shared_ptr<struct _LuaMemberVar>;
using LuaStmt = shared_ptr<struct _LuaStmt>;
using LuaReturnStmt = shared_ptr<struct _LuaReturnStmt>;
using LuaBreakStmt = shared_ptr<struct _LuaBreakStmt>;
using LuaForStmt = shared_ptr<struct _LuaForStmt>;
using LuaIfStmt = shared_ptr<struct _LuaIfStmt>;
using LuaChunk = shared_ptr<struct _LuaChunk>;
using LuaTableconstructor = shared_ptr<struct _LuaTableconstructor>;
using LuaFunction = shared_ptr<struct _LuaFunction>;

namespace lua {
namespace rt {

using nil = monostate;

using cfunction_p = shared_ptr<struct cfunction>;
using lfunction_p = shared_ptr<struct lfunction>;
using table_p = shared_ptr<struct table>;
using vallist_p = shared_ptr<struct vallist>;

using val = variant<nil, bool, double, string, cfunction_p, table_p, vallist_p, lfunction_p>;

struct table : public unordered_map<val, val> {};
struct vallist : public vector<val> {};

struct cfunction {
    template <typename T>
    cfunction(T f) : f{f} {}
    function<vallist(const vallist&)> f;
};

struct lfunction {
    lfunction(const LuaChunk& f, const vallist& params) : f{f}, params{params} {}
    LuaChunk f;
    vallist params;
};

using eval_result_t = variant<val, string>;
ostream& operator<<(ostream& os, const val& value);

struct ASTEvaluator;
struct Environment;
}
}

#define VISITABLE \
virtual lua::rt::eval_result_t accept(const lua::rt::ASTEvaluator& visitor, lua::rt::Environment& environment, bool rvalue = true) const

#define VISITABLE_IMPL(T) \
lua::rt::eval_result_t T::accept(const lua::rt::ASTEvaluator& visitor, lua::rt::Environment& environment, bool rvalue) const { \
    return visitor.visit(*this, environment, rvalue); \
}

struct _LuaAST {
    VISITABLE = 0;
};

struct _LuaExp : public _LuaAST {
    VISITABLE = 0;
    virtual ~_LuaExp() = default;
};

struct _LuaName : public _LuaExp {
    VISITABLE override;
    _LuaName(const LuaToken& token) : token {token} {}

    LuaToken token;
};

struct _LuaOp : public _LuaExp {
    VISITABLE override;
    LuaExp lhs;
    LuaExp rhs;
    LuaToken op;
};

struct _LuaUnop : public _LuaExp {
    VISITABLE override;
    LuaExp exp;
    LuaToken op;
};

struct _LuaExplist : public _LuaAST {
    VISITABLE override;
    vector<LuaExp> exps;
};

struct _LuaValue : public _LuaExp {
    VISITABLE override;
    _LuaValue(const LuaToken& token) : token {token} {}

    static LuaValue True() {
        LuaToken tok {LuaToken::Type::TRUE, "true"};
        return make_shared<_LuaValue>(tok);
    }

    static LuaValue Int(int num) {
        LuaToken tok {LuaToken::Type::NUMLIT, to_string(num)};
        return make_shared<_LuaValue>(tok);
    }

    LuaToken token;
};

struct _LuaVar : public _LuaExp {
    VISITABLE override = 0;
};

struct _LuaNameVar : public _LuaVar {
    VISITABLE override;
    _LuaNameVar(const LuaName& name) : name {name} {}

    LuaName name;
};

struct _LuaIndexVar : public _LuaVar {
    VISITABLE override;
    LuaExp table;
    LuaExp index;
};

struct _LuaMemberVar : public _LuaVar {
    VISITABLE override;
    LuaExp table;
    LuaName member;
};

struct _LuaStmt : public _LuaAST {
    VISITABLE override = 0;
    virtual ~_LuaStmt() = default;
};

struct _LuaAssignment : public _LuaStmt {
    VISITABLE override;
    LuaExplist varlist;
    LuaExplist explist;
};

struct _LuaFunctioncall : public _LuaExp, _LuaStmt {
    VISITABLE override;
    LuaExp function;
    LuaExplist args;
};

struct _LuaReturnStmt : public _LuaStmt {
    VISITABLE override;
    _LuaReturnStmt() = default;
    _LuaReturnStmt(const LuaExplist& explist) : explist {explist} {}

    LuaExplist explist;
};

struct _LuaBreakStmt : public _LuaStmt {
    VISITABLE override;

};

struct _LuaForStmt : public _LuaStmt {
    VISITABLE override;

    LuaName var;
    LuaExp start;
    LuaExp end;
    LuaExp step;

    LuaChunk body;
};

struct _LuaIfStmt : public _LuaStmt {
    VISITABLE override;

    vector<pair<LuaExp, LuaChunk>> branches;
};

struct _LuaChunk : public _LuaAST {
    VISITABLE override;

    vector<LuaStmt> statements;
};

struct _LuaTableconstructor : public _LuaExp {
    VISITABLE override;

};

struct _LuaFunction : public _LuaExp {
    VISITABLE override;

    LuaExplist params;
    LuaChunk body;
};

#endif // LUAAST_H
