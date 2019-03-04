#include "include/luaparser.h"
#include "include/luainterpreter.h"

using namespace std;

auto main(int argc, char *argv[]) -> int {
//    string program = "for i=1, 10, 1 do \n    print('hello world ', i)\nend";
//    string program = "for i=1, 2 + 4 * 2, 1 do \n    print('hello world ' .. i)\nend";
//    string program = "print('a ' .. \"b\", 5%2, (2+4)-1, 1*2*3/5)\nend";
//    string program = "a = 3\nb=4\nprint(a+b)";
    string program = "mult = function(a, b) return a*b end print(mult(2, 3))";
//    string program = "function test() for i=1, 10 do return i end end print(test())";
//    string program = "function test() for i=1, 10 do if i == 5 then return i end end end print(test())";
//    string program = "function test() while true do if i == 5 then break end end return 1, 2 end print(test())";

    LuaParser parser;
    const auto result = parser.parse(program);

    if (holds_alternative<string>(result)) {
        cerr << "Error: " << get<string>(result) << endl;
    } else {
        auto ast = get<LuaChunk>(result);
        lua::rt::Environment env;
        lua::rt::ASTEvaluator eval;

        env.populate_stdlib();

        if (auto eval_result = ast->accept(eval, env); holds_alternative<string>(eval_result)) {
            cerr << "Error: " << get<string>(eval_result) << endl;
        }
    }

    return 0;
}
