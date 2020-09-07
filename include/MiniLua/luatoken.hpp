#ifndef LUATOKEN_H
#define LUATOKEN_H

#include <iostream>
#include <string>

using namespace std;

struct LuaToken {
    // clang-format off
    enum Type {
        NONE,
        ADD, SUB, MUL, DIV, MOD, POW, LEN, STRIP, EVAL, //+, -, *, /, %, ^, #, $, "\"
        EQ, NEQ, LEQ, GEQ, LT, GT, ASSIGN, //==, ~=, <=, >=, <, >, =
        LCB, RCB, LRB, RRB, LSB, RSB, //{, }, (, ), [, ]
        SEM, COLON, COMMA, DOT, CONCAT, ELLIPSE, //;, :, ,, ., .., ...

        AND, BREAK, DO, ELSE, ELSEIF,
        END, FALSE, FOR, FUNCTION, IF,
        IN, LOCAL, NIL, NOT, OR,
        REPEAT, RETURN, THEN, TRUE, UNTIL, WHILE,

        NAME, STRINGLIT, NUMLIT, COMMENT
    } type;
    //clang-format on
    string match;
    long pos = string::npos;
    long length = 0;
    string ws = "";

    string to_string() const;
    friend ostream& operator<<(ostream& os, const LuaToken& token);
};

#endif
