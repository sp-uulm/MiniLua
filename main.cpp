#include "include/luaparser.h"
#include "include/luainterpreter.h"

using namespace std;

struct val2 : variant<bool> {
    int i = 123;

    template <typename T>
    val2(T&& v) : variant<bool> {} {}
};

auto main(int argc, char *argv[]) -> int {

//    string program = "for i=1, 10, 1 do \n    print('hello world ', i)\nend";
//    string program = "for i=1, 2 + 4 * 2, 1 do \n    print('hello world ' .. i)\nend";
//    string program = "print('a ' .. \"b\", 5%2, (2+4)-1, 1*2*3/5)\nend";
//    string program = "a = 3\nb=4\nprint(a+b)";
//    string program = "a,b = 3,4\nb,a=a,b\nprint(a-b)";
//    string program = "mult = function(a, b) return a*b end print(mult(2, 3))";
//    string program = "function test() for i=1, 10 do return i, 2 end end print(test())";
//    string program = "function test() for i=1, 10 do if i == 5 then return i end end end print(test())";
//    string program = "for i=1, 5 do print(i) if i==2 then break end end";
//    string program = "b = -1 while not (b > 5) do a=0 repeat a=a+1 if a ~= b then print(a, b) else break end until a == 10 b = b+1 end";
//    string program = "force(2, 3)";
//    string program = "i=(function() return 2 end)()+0.5; force(i, 3)";
    string program = "i=2; force(-i, 3)";
//    string program = "function test() local i = 0 return function() while true do if i == 5 then break end i=i+1 end return i, 2 end end b=test() i="a" print(i, b())";

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
