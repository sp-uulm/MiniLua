#ifndef LUAAST_H
#define LUAAST_H

#include "val.h"
#include "luatoken.h"

#include <memory>
#include <vector>
#include <functional>

using namespace std;

#define VISITABLE \
virtual lua::rt::eval_result_t accept(const lua::rt::ASTEvaluator& visitor,\
                                      const shared_ptr<lua::rt::Environment>& environment,\
                                      const lua::rt::assign_t& assign = nullopt) const

#define VISITABLE_IMPL(T) \
lua::rt::eval_result_t T::accept(const lua::rt::ASTEvaluator& visitor,\
                                 const shared_ptr<lua::rt::Environment>& environment,\
                                 const lua::rt::assign_t& assign) const { \
    \
    unsigned count = static_cast<unsigned>(get<double>(environment->getvar(string{"__visit_count"})));\
    if (count++ > get<double>(environment->getvar(string{"__visit_limit"})))\
        return string{"visit limit reached, stopping"};\
    environment->assign(string{"__visit_count"}, static_cast<double>(count), false);\
    \
    return visitor.visit(*this, environment, assign); \
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

    static LuaUnop Not(const LuaExp& exp) {
        auto result = make_shared<_LuaUnop>();
        result->exp = exp;
        result->op = {LuaToken::Type::NOT, "not"};
        return result;
    }

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

    static LuaValue Value(const LuaToken& token) {
        return make_shared<_LuaValue>(token);
    }

    static LuaValue True() {
        LuaToken tok {LuaToken::Type::TRUE, "true"};
        return Value(tok);
    }

    static LuaValue Int(int num) {
        LuaToken tok {LuaToken::Type::NUMLIT, to_string(num)};
        return Value(tok);
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

    vector<LuaToken> tokens;
};

struct _LuaAssignment : public _LuaStmt {
    VISITABLE override;
    LuaExplist varlist;
    LuaExplist explist;
    bool local = false;
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

struct _LuaLoopStmt : public _LuaStmt {
    VISITABLE override;

    bool head_controlled = true;
    LuaExp end;

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

    vector<LuaField> fields;
    vector<LuaToken> tokens;
};

struct _LuaField : public _LuaAST {
    VISITABLE override;

    LuaExp lhs;
    LuaExp rhs;
};

struct _LuaFunction : public _LuaExp {
    VISITABLE override;

    LuaExplist params;
    LuaChunk body;
};

#endif // LUAAST_H
