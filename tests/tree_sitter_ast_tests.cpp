#include "MiniLua/tree_sitter_ast.hpp"
#include "tree_sitter/tree_sitter.hpp"
#include <catch2/catch.hpp>
#include <iostream>
#include <type_traits>
namespace minilua::details {
TEST_CASE("statements", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "i,t,l = 5\n"
                         "local z = 42\n"
                         "do\n"
                         "z = i+k\n"
                         "end\n"
                         "if z>k then\n"
                         "else\n"
                         "end\n"
                         "while z>k do\n"
                         "z = z-1\n"
                         "end\n"
                         "repeat\n"
                         "z = z*k\n"
                         "until z> k^10\n"
                         "for l = 1,9 do\n"
                         "z = z-l\n"
                         "end\n"
                         "for k, v in next, t, nil do\n"
                         "  print(k, v)\n"
                         "end\n"
                         "goto alpha\n"
                         "break\n"
                         "::alpha::\n"
                         "function foo (f,o,o)\n"
                         "return f,o*o\n"
                         "end\n"
                         "local function foo (f,o,oo)\n"
                         "return f,o*oo\n"
                         "end\n"
                         "foo(i,k,z)\n"
                         "function (a,b)\n"
                         "print(a .. b)\n"
                         "end\n";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    assert(!body.ret().has_value());
    vector<Statement> statement = body.statements();
    assert(statement.size() == 15);
    long unsigned int statement_count = statement.size();
    // this loop tests if each statement got parsed to the right Class
    for (long unsigned int i = 0; i < statement_count; i++) {
        assert(statement.at(i).options().index() == i);
    }
}
TEST_CASE("expressions", "[tree-sitter][!hide]") {
    uint exp_count = 29;
    ts::Parser parser;
    std::string source = "...\n"
                         "next\n"
                         "function (a,b)\n"
                         "  foo()\n"
                         "end\n"
                         "{1,2,3,4,5}\n"
                         "1+1\n"
                         "1-1\n"
                         "1*1\n"
                         "1/1\n"
                         "1%1\n"
                         "1^1\n"
                         "1<1\n"
                         "1>1\n"
                         "1<=1\n"
                         "1>=1\n"
                         "1==1\n"
                         "1~=1\n"
                         "1 .. a\n"
                         "true and true\n"
                         "true or true\n"
                         "1<<1\n"
                         "1>>1\n"
                         "1~1\n"
                         "1|1\n"
                         "1&1\n"
                         "1//1\n"
                         "nil\n"
                         "true\n"
                         "false\n"
                         "id\n"
                         "d = not true\n"
                         "c = -1\n"
                         "a = #table\n"
                         "b = ~a\n";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    assert(!body.ret().has_value());
    vector<Statement> statement = body.statements();
    vector<Expression> exps;
    exps.reserve(exp_count);
    std::transform(
        statement.begin(), statement.begin() + exp_count, std::back_inserter(exps),
        [](Statement statement) {
            auto opt = statement.options();
            return *std::get_if<Expression>(&opt);
        });
    auto spread = exps[0].options();
    assert(spread.index() == 0);
    auto next = exps[1].options();
    assert(next.index() == 2);
    auto func_def = exps[2].options();
    assert(func_def.index() == 3);
    auto table = exps[3].options();
    assert(table.index() == 4);
    vector<BinaryOperation> bin_ops;
    bin_ops.reserve(25);
    std::transform(
        exps.begin() + 4, exps.begin() + 25, std::back_inserter(bin_ops), [](Expression exp) {
            auto opt = exp.options();
            return *get_if<BinaryOperation>(&opt);
        });
    for (uint i = 0; i < bin_ops.size(); i++) {
        assert(
            bin_ops[i].op() ==
            (BinOpEnum)i); // Bin Operations are in the same sequence as in the BinOpEnum
    }

    auto nil = exps[25].options();
    assert(holds_alternative<Value>(nil));
    auto* temp = get_if<Value>(&nil);
    assert(temp->is_nil());
    auto _true = exps[26].options();
    assert(holds_alternative<Value>(_true));
    temp = get_if<Value>(&_true);
    assert(temp->is_bool() && temp == Value(true));
    auto* b = get_if<Bool>(&(temp->raw()));
    assert(b->operator bool());
    auto _false = exps[27].options();
    assert(holds_alternative<Value>(_false));
    temp = get_if<Value>(&_false);
    assert(temp->is_bool());
    b = get_if<Bool>(&(temp->raw()));
    assert(!(b->operator bool()));
    auto id = exps[28].options();
    assert(holds_alternative<Identifier>(id));
    Identifier temp2 = *get_if<Identifier>(&id);
    assert(temp2.str() == "id"s);
    vector<UnaryOperation> un_op;
    un_op.reserve(4);
    std::transform(
        statement.begin() + exp_count, statement.end(), std::back_inserter(un_op),
        [](Statement stat) {
            auto opt = stat.options();
            auto* vd = std::get_if<VariableDeclaration>(&opt);
            auto exps = vd->declarations();
            auto exp = exps[0].options();
            assert(holds_alternative<UnaryOperation>(exp));
            return *get_if<UnaryOperation>(&exp);
        });
    for (uint j = 0; j < un_op.size(); j++) {
        assert(un_op[j].op() == (UnOpEnum)j);
    }
}

TEST_CASE("do_statements", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "do\n"
                         "end\n"
                         "do\n"
                         "1+1\n"
                         "end\n"
                         "do\n"
                         "1+1\n"
                         "2+2\n"
                         "end\n"
                         "do\n"
                         "return 2\n"
                         "end\n"
                         "do\n"
                         "1+1\n"
                         "return 1+1\n"
                         "end";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    auto stats = body.statements();
    assert(stats.size() == 5);
    assert(!body.ret().has_value());
    std::vector<DoStatement> dos;
    std::transform(stats.begin(), stats.end(), std::back_inserter(dos), [](Statement stat) {
        auto opt = stat.options();
        assert(std::holds_alternative<DoStatement>(opt));
        return *std::get_if<DoStatement>(&opt);
    });
    assert(dos.size() == 5);
    assert(dos[0].body().statements().empty());
    assert(!dos[0].body().ret().has_value());
    assert(dos[1].body().statements().size() == 1);
    assert(!dos[1].body().ret().has_value());
    assert(dos[2].body().statements().size() == 2);
    assert(!dos[2].body().ret().has_value());
    assert(dos[3].body().statements().empty());
    assert(dos[3].body().ret().has_value());
    assert(dos[4].body().statements().size() == 1);
    assert(dos[4].body().ret().has_value());
}
TEST_CASE("if_statements", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "if c<d then\n"
                         "elseif i then\n"
                         "1+1\n"
                         "elseif false then\n"
                         "2+2\n"
                         "3+3\n"
                         "else\n"
                         "return 0\n"
                         "end\n"
                         "if d then\n"
                         "return false\n"
                         "end\n"
                         "if e then\n"
                         "a+b+c\n"
                         "else\n"
                         "foo()\n"
                         "end\n"
                         "if true then\n"
                         "else\n"
                         "1+1\n"
                         "end\n"
                         "if r then\n"
                         "1+1\n"
                         "elseif true then\n"
                         "2+2\n"
                         "elseif f then\n"
                         "3+3\n"
                         "3+3\n"
                         "end\n"
                         "if true then\n"
                         "end";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    auto stats = body.statements();
    assert(stats.size() == 6);
    std::vector<IfStatement> ifs;
    std::transform(stats.begin(), stats.end(), std::back_inserter(ifs), [](Statement stat) {
        auto opt = stat.options();
        assert(std::holds_alternative<IfStatement>(opt));
        return *std::get_if<IfStatement>(&opt);
    });
    assert(ifs.size() == 6);
    assert(holds_alternative<BinaryOperation>(ifs[0].cond().options()));
    assert(ifs[0].body().statements().empty());
    assert(!ifs[0].body().ret().has_value());
    assert(ifs[0].elseifs().size() == 2);
    assert(holds_alternative<Identifier>(ifs[0].elseifs()[0].cond().options()));
    assert(ifs[0].elseifs()[0].body().statements().size() == 1);
    assert(!ifs[0].elseifs()[0].body().ret().has_value());
    assert(holds_alternative<Value>(ifs[0].elseifs()[1].cond().options()));
    assert(ifs[0].elseifs()[1].body().statements().size() == 2);
    assert(!ifs[0].elseifs()[1].body().ret().has_value());
    assert(ifs[0].else_().has_value());
    assert(ifs[0].else_().value().body().statements().empty());
    assert(ifs[0].else_().value().body().ret().has_value());
    assert(std::holds_alternative<Value>(
        ifs[0].else_().value().body().ret().value().explist()[0].options()));
    assert(ifs[1].body().statements().empty());
    assert(ifs[1].body().ret().has_value());
    assert(ifs[1].elseifs().empty());
    assert(!ifs[1].else_().has_value());
    assert(holds_alternative<Identifier>(ifs[1].cond().options()));
    assert(holds_alternative<Identifier>(ifs[2].cond().options()));
    assert(ifs[2].body().statements().size() == 1);
    assert(!ifs[2].body().ret().has_value());
    assert(ifs[2].elseifs().empty());
    assert(ifs[2].else_().has_value());
    assert(ifs[2].else_().value().body().statements().size() == 1);
    assert(!ifs[2].else_().value().body().ret().has_value());
    assert(holds_alternative<Value>(ifs[3].cond().options()));
    assert(ifs[3].body().statements().empty());
    assert(!ifs[3].body().ret().has_value());
    assert(ifs[3].elseifs().empty());
    assert(ifs[3].else_().has_value());
    assert(ifs[3].else_().value().body().statements().size() == 1);
    assert(!ifs[3].else_().value().body().ret().has_value());
    assert(holds_alternative<Identifier>(ifs[4].cond().options()));
    assert(ifs[4].body().statements().size() == 1);
    assert(!ifs[4].body().ret().has_value());
    assert(ifs[4].elseifs().size() == 2);
    assert(!ifs[4].else_().has_value());
    assert(ifs[4].elseifs()[0].body().statements().size() == 1);
    assert(!ifs[4].elseifs()[0].body().ret().has_value());
    assert(ifs[4].elseifs()[1].body().statements().size() == 2);
    assert(!ifs[4].elseifs()[1].body().ret().has_value());
    assert(ifs[5].body().statements().empty());
    assert(!ifs[5].body().ret().has_value());
    assert(ifs[5].elseifs().empty());
    assert(!ifs[5].else_().has_value());
}
TEST_CASE("for_statements", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "for i = 1,2 do\n"
                         "  foo(12)\n"
                         "end\n"

                         "for i = 1,2 do\n"
                         "end\n"

                         "for c = a,b,42 do\n"
                         "  foo()\n"
                         "  s = a+c+s\n"
                         "  f = 36+6\n"
                         "end";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    auto stats = body.statements();
    assert(stats.size() == 3);
    assert(!body.ret().has_value());
    std::vector<ForStatement> fors;
    std::transform(stats.begin(), stats.end(), std::back_inserter(fors), [](Statement stat) {
        auto opt = stat.options();
        assert(std::holds_alternative<ForStatement>(opt));
        return *std::get_if<ForStatement>(&opt);
    });
    assert(fors.size() == 3);
    // 1st loop
    assert(fors[0].body().statements().size() == 1);
    assert(!fors[0].body().ret().has_value());
    assert(fors[0].loop_exp().variable().str() == "i"s);
    auto start1_opt = fors[0].loop_exp().start().options();
    assert(holds_alternative<Value>(start1_opt));
    auto* start1 = get_if<Value>(&start1_opt);
    assert(start1->is_number());
    assert(holds_alternative<Number>(start1->raw()));
    if (auto* num1 = get_if<minilua::Number>(&start1->raw())) {
        assert(num1->value == 1);
    }
    auto end1_opt = fors[0].loop_exp().end().options();
    assert(holds_alternative<Value>(end1_opt));
    auto* end1 = get_if<Value>(&end1_opt);
    assert(end1->is_number());
    assert(holds_alternative<Number>(end1->raw()));
    if (auto* num2 = get_if<minilua::Number>(&end1->raw())) {
        assert(num2->value == 2);
    }
    assert(!fors[0].loop_exp().step().has_value());
    // 2nd loop just to check if the empty body works fine here
    assert(fors[1].body().statements().empty());
    // 3rd loop
    assert(fors[2].body().statements().size() == 3);
    assert(!fors[2].body().ret().has_value());
    // checking the loopexpression
    assert(fors[2].loop_exp().variable().str() == "c"s);
    auto start3_opt = fors[2].loop_exp().start().options();
    assert(holds_alternative<Identifier>(start3_opt));
    auto start3 = get_if<Identifier>(&start3_opt);
    assert(start3->str() == "a"s);
    assert(fors[2].loop_exp().step().has_value());
    auto step3_opt = fors[2].loop_exp().step()->options();
    assert(holds_alternative<Identifier>(step3_opt));
    auto step3 = get_if<Identifier>(&step3_opt);
    assert(step3->str() == "b"s);
    auto end3_opt = fors[2].loop_exp().end().options();
    assert(holds_alternative<Value>(end3_opt));
    auto end3 = get_if<Value>(&end3_opt);
    assert(end3->is_number());
    auto end3_num = get_if<Number>(&end3->raw());
    assert(end3_num->value == 42);
}
TEST_CASE("for_in_statements", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "for k, v in next, t, nil do\n"
                         "  print(k, v)\n"
                         "end\n"
                         "for a,b,c,d,e in foo(var) do\n"
                         "  return 1\n"
                         "end";

    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    auto stats = body.statements();
    assert(stats.size() == 2);
    assert(!body.ret().has_value());
    std::vector<ForInStatement> fors;
    std::transform(stats.begin(), stats.end(), std::back_inserter(fors), [](Statement stat) {
        auto opt = stat.options();
        assert(std::holds_alternative<ForInStatement>(opt));
        return *std::get_if<ForInStatement>(&opt);
    });
    assert(fors.size() == 2);
    assert(fors[0].loop_exp().loop_vars().size() == 2);
    assert(fors[0].loop_exp().loop_exps().size() == 3);
    assert(fors[1].loop_exp().loop_vars().size() == 5);
    assert(fors[1].loop_exp().loop_exps().size() == 1);
}
TEST_CASE("function_statements", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "function foo (a,b,c)\n"
                         "  1+1\n"
                         "  return 3+3\n"
                         "end\n"
                         "function table.prop1.prop2.prop3:method (a,b,c)\n"
                         "  return true\n"
                         "end\n"
                         "function foo(self,...)\n"
                         "end\n"
                         "function foo(...)\n"
                         "end\n"
                         "function foo(self)\n"
                         "end\n"
                         "function foo (self,a,b,c)\n"
                         "end\n"
                         "function foo (a,b,c,...)\n"
                         "end\n"
                         "function foo (...,a,b,c)\n"
                         "end\n"
                         "function foo(self,a,b,c,...)\n"
                         "end\n"
                         "function foo()\n"
                         "end";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    auto stats = body.statements();
    assert(stats.size() == 10);
    assert(!body.ret().has_value());
    std::vector<FunctionStatement> func;
    std::transform(stats.begin(), stats.end(), std::back_inserter(func), [](Statement stat) {
        auto opt = stat.options();
        assert(std::holds_alternative<FunctionStatement>(opt));
        return *std::get_if<FunctionStatement>(&opt);
    });
    assert(func.size() == 10);
    assert(func[1].name().method().has_value());
    assert(func[1].name().identifier().size() == 4);
    assert(func[0].name().identifier().size() == 1);
    std::vector<string> vec{"a", "b", "c"};
    std::vector<string> params;
    vector<Identifier> identifiers;
    for (uint i = 0; i < 9; i++) {
        if (i < 2 || i > 4) {
            assert(func[i].parameters().params().size() == 3);
            params.clear();
            identifiers = func[i].parameters().params();
            std::transform(
                identifiers.begin(), identifiers.end(), std::back_inserter(params),
                [](Identifier id) { return id.str(); });
            assert(params == vec);
        } else {
            assert(func[i].parameters().params().empty());
        }
        switch (i) {
        case 0:
        case 1:
            assert(!func[i].parameters().leading_self());
            assert(func[i].parameters().spread() == NO_SPREAD);
            break;
        case 2:
            assert(func[i].parameters().leading_self());
            assert(func[i].parameters().spread() == END);
            break;
        case 3:
            assert(!func[i].parameters().leading_self());
            assert(func[i].parameters().spread() == BEGIN);
            break;
        case 4:
        case 5:
            assert(func[i].parameters().leading_self());
            assert(func[i].parameters().spread() == NO_SPREAD);
            break;
        case 6:
            assert(!func[i].parameters().leading_self());
            assert(func[i].parameters().spread() == END);
            break;
        case 7:
            assert(!func[i].parameters().leading_self());
            assert(func[i].parameters().spread() == BEGIN);
            break;
        case 8:
            assert(func[i].parameters().leading_self());
            assert(func[i].parameters().spread() == END);
            break;
        }
    }
    assert(!func[9].parameters().leading_self());
    assert(func[9].parameters().spread() == NO_SPREAD);
    assert(func[9].parameters().params().empty());
}
TEST_CASE("while_and_repeat_statements", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "while i<#table do\n"
                         "  i=i+1\n"
                         "end\n"
                         "while i<#table do\n"
                         "end\n"
                         ""
                         "repeat\n"
                         "until i>42\n"
                         "repeat\n"
                         "  i=i*9;"
                         "  return 1,2,3\n"
                         "until b\n";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    auto stats = body.statements();
    assert(stats.size() == 4);
    auto opt1 = stats[0].options();
    assert(std::holds_alternative<WhileStatement>(opt1));
    auto while_stat = std::get_if<WhileStatement>(&opt1);
    assert(holds_alternative<BinaryOperation>(while_stat->exit_cond().options()));
    assert(while_stat->body().statements().size() == 1);
    assert(!while_stat->body().ret().has_value());
    auto opt2 = stats[1].options();
    assert(std::holds_alternative<WhileStatement>(opt2));
    while_stat = std::get_if<WhileStatement>(&opt2);
    assert(holds_alternative<BinaryOperation>(while_stat->exit_cond().options()));
    assert(while_stat->body().statements().empty());
    assert(!while_stat->body().ret().has_value());
    auto opt3 = stats[2].options();
    assert(std::holds_alternative<RepeatStatement>(opt3));
    auto repeat_stat = std::get_if<RepeatStatement>(&opt3);
    assert(holds_alternative<BinaryOperation>(repeat_stat->until_cond().options()));
    assert(repeat_stat->body().statements().empty());
    assert(!repeat_stat->body().ret().has_value());
    auto opt4 = stats[3].options();
    assert(std::holds_alternative<RepeatStatement>(opt4));
    repeat_stat = std::get_if<RepeatStatement>(&opt4);
    assert(holds_alternative<Identifier>(repeat_stat->until_cond().options()));
    assert(repeat_stat->body().statements().size() == 1);
    assert(repeat_stat->body().ret().has_value());
}
TEST_CASE("return_statements", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "do\n"
                         "return a,b,c,d;\n"
                         "end\n"
                         "do\n"
                         "return\n"
                         "end\n"
                         "do\n"
                         "return a\n"
                         "end";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    auto stats = body.statements();
    assert(stats.size() == 3);
    std::vector<Return> returns;
    std::transform(stats.begin(), stats.end(), std::back_inserter(returns), [](Statement stat) {
        auto opt = stat.options();
        assert(std::holds_alternative<DoStatement>(opt));
        auto do_stat = *std::get_if<DoStatement>(&opt);
        assert(do_stat.body().ret().has_value());
        return do_stat.body().ret().value();
    });
    assert(returns[0].explist().size() == 4);
    for (uint i = 0; i < 4; i++) {
        assert(std::holds_alternative<Identifier>(returns[0].explist()[i].options()));
    }
    assert(returns[1].explist().empty());
    assert(returns[2].explist().size() == 1);
    assert(std::holds_alternative<Identifier>(returns[2].explist()[0].options()));
}
TEST_CASE("var_dec_statements", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "a = 1\n"
                         "a,b,c,d = 1+2,5+7,a\n"
                         "local e\n"
                         "local f,g,h\n"
                         "local i,j = 42,96\n"
                         "\n"
                         "table1.table2.field1 = function() print(2) end\n"
                         "";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    auto stats = body.statements();
    assert(stats.size() == 6);
    // 1st statement
    auto opt1 = stats[0].options();
    assert(std::holds_alternative<VariableDeclaration>(opt1));
    auto var_dec1 = std::get_if<VariableDeclaration>(&opt1);
    assert(var_dec1->declarations().size() == 1);
    assert(var_dec1->declarators().size() == 1);
    assert(std::holds_alternative<Identifier>(var_dec1->declarators()[0].var()));
    assert(std::holds_alternative<Value>(var_dec1->declarations()[0].options()));
    // 2nd statement
    auto opt2 = stats[1].options();
    assert(std::holds_alternative<VariableDeclaration>(opt2));
    auto var_dec2 = std::get_if<VariableDeclaration>(&opt2);
    assert(var_dec2->declarations().size() == 3);
    assert(var_dec2->declarators().size() == 4);
    for (uint i = 0; i < 4; i++) {
        assert(std::holds_alternative<Identifier>(var_dec2->declarators()[i].var()));
    }
    assert(std::holds_alternative<BinaryOperation>(var_dec2->declarations()[0].options()));
    assert(std::holds_alternative<BinaryOperation>(var_dec2->declarations()[1].options()));
    assert(std::holds_alternative<Identifier>(var_dec2->declarations()[2].options()));
    // 3rd statement
    auto opt3 = stats[2].options();
    assert(std::holds_alternative<LocalVariableDeclaration>(opt3));
    auto var_dec3 = std::get_if<LocalVariableDeclaration>(&opt3);
    assert(var_dec3->declarations().empty());
    assert(var_dec3->declarators().size() == 1);
    assert(var_dec3->declarators()[0].str() == "e"s);
    // 4th statement
    auto opt4 = stats[3].options();
    assert(std::holds_alternative<LocalVariableDeclaration>(opt4));
    auto var_dec4 = std::get_if<LocalVariableDeclaration>(&opt4);
    assert(var_dec4->declarations().empty());
    assert(var_dec4->declarators().size() == 3);
    assert(var_dec4->declarators()[0].str() == "f"s);
    assert(var_dec4->declarators()[1].str() == "g"s);
    assert(var_dec4->declarators()[2].str() == "h"s);
    // 5th statement
    auto opt5 = stats[4].options();
    assert(std::holds_alternative<LocalVariableDeclaration>(opt5));
    auto var_dec5 = std::get_if<LocalVariableDeclaration>(&opt5);
    assert(var_dec5->declarations().size() == 2);
    assert(var_dec5->declarators().size() == 2);
    assert(std::holds_alternative<Value>(var_dec5->declarations()[0].options()));
    assert(std::holds_alternative<Value>(var_dec5->declarations()[1].options()));
    assert(var_dec5->declarators()[0].str() == "i"s);
    assert(var_dec5->declarators()[1].str() == "j"s);
    // 6th statement
    auto opt6 = stats[5].options();
    assert(std::holds_alternative<VariableDeclaration>(opt6));
    auto var_dec6 = std::get_if<VariableDeclaration>(&opt6);
    assert(var_dec6->declarators().size() == 1);
    auto declarator = var_dec6->declarators()[0];
    auto dec_opt1 = declarator.var();
    assert(std::holds_alternative<FieldExpression>(dec_opt1));
    auto fe1 = std::get_if<FieldExpression>(&dec_opt1);
    assert(fe1->property_id().str() == "field1");
    auto prefix1 = fe1->table_id().options();
    assert(std::holds_alternative<VariableDeclarator>(prefix1));
    auto dec2 = get_if<VariableDeclarator>(&prefix1);
    auto dec_opt2 = dec2->var();
    assert(holds_alternative<FieldExpression>(dec_opt2));
    auto fe2 = std::get_if<FieldExpression>(&dec_opt2);
    assert(fe2->property_id().str() == "table2");
    auto prefix2 = fe2->table_id().options();
    assert(std::holds_alternative<VariableDeclarator>(prefix2));
    auto dec3 = std::get_if<VariableDeclarator>(&prefix2);
    auto dec_opt3 = dec3->var();
    assert(holds_alternative<Identifier>(dec_opt3));
    auto table_id = std::get_if<Identifier>(&dec_opt3);
    assert(table_id->str() == "table1"s);
}
TEST_CASE("table_statements", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "{\n"
                         "field1 = \"name\";\n"
                         "[2+2] = {1,2,3};\n"
                         "function()\n"
                         "  return field1\n"
                         "end"
                         "}\n";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    auto stats = body.statements();
    assert(stats.size() == 1);
    auto opt1 = stats[0].options();
    assert(std::holds_alternative<Expression>(opt1));
    auto* exp1 = std::get_if<Expression>(&opt1);
    auto exp_opt1 = exp1->options();
    assert(std::holds_alternative<Table>(exp_opt1));
    auto table1 = std::get_if<Table>(&exp_opt1);
    auto fields = table1->fields();
    assert(fields.size() == 3);
    auto field1_opt = fields[0].content();
    assert(std::holds_alternative<ID_FIELD>(field1_opt));
    auto field1 = std::get_if<ID_FIELD>(&field1_opt);
    assert(field1->first.str() == "field1"s);
    assert(std::holds_alternative<Value>(field1->second.options()));
    auto content1_opt = field1->second.options();
    auto content1 = std::get_if<Value>(&content1_opt);
    assert(content1->is_string());
    auto raw = content1->raw();
    auto content1_str = get_if<String>(&raw);
    assert(content1_str->value == "name"s);
    auto field2_opt = fields[1].content();
    assert(std::holds_alternative<INDEX_FIELD>(field2_opt));
    auto field2 = std::get_if<INDEX_FIELD>(&field2_opt);
    assert(std::holds_alternative<BinaryOperation>(field2->first.options()));
    assert(std::holds_alternative<Table>(field2->second.options()));
    auto field3_opt = fields[2].content();
    assert(std::holds_alternative<Expression>(field3_opt));
    auto field3 = std::get_if<Expression>(&field3_opt);
    auto content3_opt = field3->options();
    assert(std::holds_alternative<FunctionDefinition>(content3_opt));
}
TEST_CASE("function_calls", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "foo(a,b)\n"
                         "table.foo()\n"
                         "table:foo()\n"
                         "foo \"abc\"\n"
                         "table:foo {1,2,3,4}";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    auto stats = body.statements();
    assert(stats.size() == 5);
    std::vector<FunctionCall> func_calls;
    std::transform(stats.begin(), stats.end(), std::back_inserter(func_calls), [](Statement stat) {
        auto opt = stat.options();
        assert(std::holds_alternative<FunctionCall>(opt));
        return *std::get_if<FunctionCall>(&opt);
    });
    assert(!func_calls[0].method().has_value());
    assert(std::holds_alternative<VariableDeclarator>(func_calls[0].id().options()));
    auto opt1 = func_calls[0].id().options();
    auto name1 = std::get_if<VariableDeclarator>(&opt1);
    assert(holds_alternative<Identifier>(name1->var()));
    assert(func_calls[0].args().size() == 2);

    assert(!func_calls[1].method().has_value());
    assert(std::holds_alternative<VariableDeclarator>(func_calls[1].id().options()));
    auto opt2 = func_calls[1].id().options();
    auto name2 = std::get_if<VariableDeclarator>(&opt2);
    assert(holds_alternative<FieldExpression>(name2->var()));
    assert(func_calls[1].args().empty());

    assert(func_calls[2].method().has_value());
    assert(std::holds_alternative<VariableDeclarator>(func_calls[2].id().options()));
    auto opt3 = func_calls[2].id().options();
    auto name3 = std::get_if<VariableDeclarator>(&opt3);
    assert(holds_alternative<Identifier>(name3->var()));
    assert(func_calls[2].args().empty());

    assert(func_calls[3].args().size() == 1);
    assert(std::holds_alternative<Value>(func_calls[3].args()[0].options()));
    auto opt4 = func_calls[3].args()[0].options();
    auto str = std::get_if<Value>(&opt4);
    assert(str->is_string());

    assert(func_calls[4].args().size() == 1);
    assert(std::holds_alternative<Table>(func_calls[4].args()[0].options()));
    auto opt5 = func_calls[4].args()[0].options();
    auto table = std::get_if<Table>(&opt5);
    assert(table->fields().size() == 4);
    for (uint i = 0; i < 4; i++) {
        assert(std::holds_alternative<Expression>(table->fields()[i].content()));
    }
}
} // namespace minilua::details