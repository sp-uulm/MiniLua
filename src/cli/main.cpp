#include "luaparser.h"
#include "luainterpreter.h"
#include <chrono>

using namespace std;

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
//    string program = "i=1+1.5; force(-i, 3)";
//    string program = "a = {1, 2, 3, [5] = 'foo'; bar = true, [5 == 18] = {}}";
//    string program = "a = {4, 5, 6}; print(a[2])";
//    string program = "a={}; a[1] = 2";
//    string program = "a = {}; a['foo'] = 5; print(a['foo'])";
//    string program = "a = {foo = 'bar'} print(a.foo)";
//    string program = "a = {foo = {'bar'}} print(a.foo[1])";
//    string program = "a = {} a.foo = 5 print(a.foo)";
//    string program = "a=2 if true then local a=3 print(a) end print(a)";
//    string program = "local function test() local i = 0 return function() while true do if i == 5 then break end i=i+1 end return i, 2 end end b=test() i=\"a\" print(i, b())";
//    string program = "a = 3 print(_G._G._G._G._G.a)";
//    string program = "a = 3\\";
/*string program = R"-(
                 function f(x)
                    return math.sin(x)
                 end

                 a = {}
                 for i=1, 20, 1 do
                    a[#a + 1] = f(i)\;
                 end
                 )-";*/

string program = R"-(
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 _G.print(0,0,0)
                 )-";

/*string program = R"-(
    print('hello world')

    a = {1,2,3,[5] = 5}

    a[4] = "foo"

    print(#a)
    )-";*/

    auto parse_start = std::chrono::steady_clock::now();
    LuaParser parser;
    const auto result = parser.parse(program);
    auto parse_end = std::chrono::steady_clock::now();

    if (holds_alternative<string>(result)) {
        cerr << "Error: " << get<string>(result) << endl;
    } else {
        auto eval_start = std::chrono::steady_clock::now();
        auto ast = get<LuaChunk>(result);
        auto env = make_shared<lua::rt::Environment>(nullptr);
        lua::rt::ASTEvaluator eval;

        env->populate_stdlib();
        auto stdlib_end = std::chrono::steady_clock::now();

        if (auto eval_result = ast->accept(eval, env); holds_alternative<string>(eval_result)) {
            cerr << "Error: " << get<string>(eval_result) << endl;
        } else {
            auto eval_end = std::chrono::steady_clock::now();
            if (auto sc = get_sc(eval_result)) {
                auto new_program = get_string((*sc)->apply(parser.tokens));
                auto apply_end = std::chrono::steady_clock::now();

                cout << "Source changes: " << (*sc)->to_string() << endl;
                cout << "New program: " << new_program << endl;

                cout << "Parse [µs]: " << std::chrono::duration_cast<std::chrono::microseconds>(parse_end - parse_start).count() << endl;
                cout << "Execute [µs]: " << std::chrono::duration_cast<std::chrono::microseconds>(eval_end - eval_start).count() << endl;
                cout << "Apply SC [µs]: " << std::chrono::duration_cast<std::chrono::microseconds>(apply_end - eval_end).count() << endl;

                cout << "Total time [µs]: " << std::chrono::duration_cast<std::chrono::microseconds>(apply_end - parse_start).count() << endl;
            }
        }

        env->clear();
    }

    return 0;
}
