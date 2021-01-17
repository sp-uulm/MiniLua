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
    CHECK(!body.ret().has_value());
    vector<Statement> statement = body.statements();
    CHECK(statement.size() == 15);
    long unsigned int statement_count = statement.size();
    // this loop tests if each statement got parsed to the right Class
    for (long unsigned int i = 0; i < statement_count; i++) {
        CHECK(statement.at(i).options().index() == i);
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
    CHECK(!body.ret().has_value());
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
    CHECK(spread.index() == 0);
    auto next = exps[1].options();
    CHECK(next.index() == 2);
    auto func_def = exps[2].options();
    CHECK(func_def.index() == 3);
    auto table = exps[3].options();
    CHECK(table.index() == 4);
    vector<BinaryOperation> bin_ops;
    bin_ops.reserve(25);
    std::transform(
        exps.begin() + 4, exps.begin() + 25, std::back_inserter(bin_ops), [](Expression exp) {
            auto opt = exp.options();
            return *get_if<BinaryOperation>(&opt);
        });
    for (uint i = 0; i < bin_ops.size(); i++) {
        CHECK(
            bin_ops[i].op() ==
            (BinOpEnum)i); // Bin Operations are in the same sequence as in the BinOpEnum
    }

    auto nil = exps[25].options();
    CHECK(holds_alternative<Value>(nil));
    auto* temp = get_if<Value>(&nil);
    CHECK(temp->is_nil());
    auto _true = exps[26].options();
    CHECK(holds_alternative<Value>(_true));
    temp = get_if<Value>(&_true);
    CHECK(temp->is_bool());
    CHECK(temp == Value(true));
    auto* b = get_if<Bool>(&(temp->raw()));
    CHECK(b->operator bool());
    auto _false = exps[27].options();
    CHECK(holds_alternative<Value>(_false));
    temp = get_if<Value>(&_false);
    CHECK(temp->is_bool());
    b = get_if<Bool>(&(temp->raw()));
    CHECK(!(b->operator bool()));
    auto id = exps[28].options();
    CHECK(holds_alternative<Identifier>(id));
    Identifier temp2 = *get_if<Identifier>(&id);
    CHECK(temp2.str() == "id"s);
    vector<UnaryOperation> un_op;
    un_op.reserve(4);
    std::transform(
        statement.begin() + exp_count, statement.end(), std::back_inserter(un_op),
        [](Statement stat) {
            auto opt = stat.options();
            auto* vd = std::get_if<VariableDeclaration>(&opt);
            auto exps = vd->declarations();
            auto exp = exps[0].options();
            CHECK(holds_alternative<UnaryOperation>(exp));
            return *get_if<UnaryOperation>(&exp);
        });
    for (uint j = 0; j < un_op.size(); j++) {
        CHECK(un_op[j].op() == (UnOpEnum)j);
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
    CHECK(stats.size() == 5);
    CHECK(!body.ret().has_value());
    std::vector<DoStatement> dos;
    std::transform(stats.begin(), stats.end(), std::back_inserter(dos), [](Statement stat) {
        auto opt = stat.options();
        CHECK(std::holds_alternative<DoStatement>(opt));
        return *std::get_if<DoStatement>(&opt);
    });
    CHECK(dos.size() == 5);
    CHECK(dos[0].body().statements().empty());
    CHECK(!dos[0].body().ret().has_value());
    CHECK(dos[1].body().statements().size() == 1);
    CHECK(!dos[1].body().ret().has_value());
    CHECK(dos[2].body().statements().size() == 2);
    CHECK(!dos[2].body().ret().has_value());
    CHECK(dos[3].body().statements().empty());
    CHECK(dos[3].body().ret().has_value());
    CHECK(dos[4].body().statements().size() == 1);
    CHECK(dos[4].body().ret().has_value());
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
    CHECK(stats.size() == 6);
    std::vector<IfStatement> ifs;
    std::transform(stats.begin(), stats.end(), std::back_inserter(ifs), [](Statement stat) {
        auto opt = stat.options();
        CHECK(std::holds_alternative<IfStatement>(opt));
        return *std::get_if<IfStatement>(&opt);
    });
    CHECK(ifs.size() == 6);
    CHECK(holds_alternative<BinaryOperation>(ifs[0].cond().options()));
    CHECK(ifs[0].body().statements().empty());
    CHECK(!ifs[0].body().ret().has_value());
    CHECK(ifs[0].elseifs().size() == 2);
    CHECK(holds_alternative<Identifier>(ifs[0].elseifs()[0].cond().options()));
    CHECK(ifs[0].elseifs()[0].body().statements().size() == 1);
    CHECK(!ifs[0].elseifs()[0].body().ret().has_value());
    CHECK(holds_alternative<Value>(ifs[0].elseifs()[1].cond().options()));
    CHECK(ifs[0].elseifs()[1].body().statements().size() == 2);
    CHECK(!ifs[0].elseifs()[1].body().ret().has_value());
    CHECK(ifs[0].else_().has_value());
    CHECK(ifs[0].else_().value().body().statements().empty());
    CHECK(ifs[0].else_().value().body().ret().has_value());
    CHECK(std::holds_alternative<Value>(
        ifs[0].else_().value().body().ret().value().explist()[0].options()));
    CHECK(ifs[1].body().statements().empty());
    CHECK(ifs[1].body().ret().has_value());
    CHECK(ifs[1].elseifs().empty());
    CHECK(!ifs[1].else_().has_value());
    CHECK(holds_alternative<Identifier>(ifs[1].cond().options()));
    CHECK(holds_alternative<Identifier>(ifs[2].cond().options()));
    CHECK(ifs[2].body().statements().size() == 1);
    CHECK(!ifs[2].body().ret().has_value());
    CHECK(ifs[2].elseifs().empty());
    CHECK(ifs[2].else_().has_value());
    CHECK(ifs[2].else_().value().body().statements().size() == 1);
    CHECK(!ifs[2].else_().value().body().ret().has_value());
    CHECK(holds_alternative<Value>(ifs[3].cond().options()));
    CHECK(ifs[3].body().statements().empty());
    CHECK(!ifs[3].body().ret().has_value());
    CHECK(ifs[3].elseifs().empty());
    CHECK(ifs[3].else_().has_value());
    CHECK(ifs[3].else_().value().body().statements().size() == 1);
    CHECK(!ifs[3].else_().value().body().ret().has_value());
    CHECK(holds_alternative<Identifier>(ifs[4].cond().options()));
    CHECK(ifs[4].body().statements().size() == 1);
    CHECK(!ifs[4].body().ret().has_value());
    CHECK(ifs[4].elseifs().size() == 2);
    CHECK(!ifs[4].else_().has_value());
    CHECK(ifs[4].elseifs()[0].body().statements().size() == 1);
    CHECK(!ifs[4].elseifs()[0].body().ret().has_value());
    CHECK(ifs[4].elseifs()[1].body().statements().size() == 2);
    CHECK(!ifs[4].elseifs()[1].body().ret().has_value());
    CHECK(ifs[5].body().statements().empty());
    CHECK(!ifs[5].body().ret().has_value());
    CHECK(ifs[5].elseifs().empty());
    CHECK(!ifs[5].else_().has_value());
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
    CHECK(stats.size() == 3);
    CHECK(!body.ret().has_value());
    std::vector<ForStatement> fors;
    std::transform(stats.begin(), stats.end(), std::back_inserter(fors), [](Statement stat) {
        auto opt = stat.options();
        CHECK(std::holds_alternative<ForStatement>(opt));
        return *std::get_if<ForStatement>(&opt);
    });
    CHECK(fors.size() == 3);
    // 1st loop
    CHECK(fors[0].body().statements().size() == 1);
    CHECK(!fors[0].body().ret().has_value());
    CHECK(fors[0].loop_exp().variable().str() == "i"s);
    auto start1_opt = fors[0].loop_exp().start().options();
    CHECK(holds_alternative<Value>(start1_opt));
    auto* start1 = get_if<Value>(&start1_opt);
    CHECK(start1->is_number());
    CHECK(holds_alternative<Number>(start1->raw()));
    if (auto* num1 = get_if<minilua::Number>(&start1->raw())) {
        CHECK(num1->value == 1);
    }
    auto end1_opt = fors[0].loop_exp().end().options();
    CHECK(holds_alternative<Value>(end1_opt));
    auto* end1 = get_if<Value>(&end1_opt);
    CHECK(end1->is_number());
    CHECK(holds_alternative<Number>(end1->raw()));
    if (auto* num2 = get_if<minilua::Number>(&end1->raw())) {
        CHECK(num2->value == 2);
    }
    CHECK(!fors[0].loop_exp().step().has_value());
    // 2nd loop just to check if the empty body works fine here
    CHECK(fors[1].body().statements().empty());
    // 3rd loop
    CHECK(fors[2].body().statements().size() == 3);
    CHECK(!fors[2].body().ret().has_value());
    // checking the loopexpression
    CHECK(fors[2].loop_exp().variable().str() == "c"s);
    auto start3_opt = fors[2].loop_exp().start().options();
    CHECK(holds_alternative<Identifier>(start3_opt));
    auto start3 = get_if<Identifier>(&start3_opt);
    CHECK(start3->str() == "a"s);
    CHECK(fors[2].loop_exp().step().has_value());
    auto step3_opt = fors[2].loop_exp().step()->options();
    CHECK(holds_alternative<Identifier>(step3_opt));
    auto step3 = get_if<Identifier>(&step3_opt);
    CHECK(step3->str() == "b"s);
    auto end3_opt = fors[2].loop_exp().end().options();
    CHECK(holds_alternative<Value>(end3_opt));
    auto end3 = get_if<Value>(&end3_opt);
    CHECK(end3->is_number());
    auto end3_num = get_if<Number>(&end3->raw());
    CHECK(end3_num->value == 42);
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
    CHECK(stats.size() == 2);
    CHECK(!body.ret().has_value());
    std::vector<ForInStatement> fors;
    std::transform(stats.begin(), stats.end(), std::back_inserter(fors), [](Statement stat) {
        auto opt = stat.options();
        CHECK(std::holds_alternative<ForInStatement>(opt));
        return *std::get_if<ForInStatement>(&opt);
    });
    CHECK(fors.size() == 2);
    CHECK(fors[0].loop_exp().loop_vars().size() == 2);
    CHECK(fors[0].loop_exp().loop_exps().size() == 3);
    CHECK(fors[1].loop_exp().loop_vars().size() == 5);
    CHECK(fors[1].loop_exp().loop_exps().size() == 1);
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
    CHECK(stats.size() == 10);
    CHECK(!body.ret().has_value());
    std::vector<FunctionStatement> func;
    std::transform(stats.begin(), stats.end(), std::back_inserter(func), [](Statement stat) {
        auto opt = stat.options();
        CHECK(std::holds_alternative<FunctionStatement>(opt));
        return *std::get_if<FunctionStatement>(&opt);
    });
    CHECK(func.size() == 10);
    CHECK(func[1].name().method().has_value());
    CHECK(func[1].name().identifier().size() == 4);
    CHECK(func[0].name().identifier().size() == 1);
    std::vector<string> vec{"a", "b", "c"};
    std::vector<string> params;
    vector<Identifier> identifiers;
    for (uint i = 0; i < 9; i++) {
        if (i < 2 || i > 4) {
            CHECK(func[i].parameters().params().size() == 3);
            params.clear();
            identifiers = func[i].parameters().params();
            std::transform(
                identifiers.begin(), identifiers.end(), std::back_inserter(params),
                [](Identifier id) { return id.str(); });
            CHECK(params == vec);
        } else {
            CHECK(func[i].parameters().params().empty());
        }
        switch (i) {
        case 0:
        case 1:
            CHECK(!func[i].parameters().leading_self());
            CHECK(func[i].parameters().spread() == NO_SPREAD);
            break;
        case 2:
            CHECK(func[i].parameters().leading_self());
            CHECK(func[i].parameters().spread() == END);
            break;
        case 3:
            CHECK(!func[i].parameters().leading_self());
            CHECK(func[i].parameters().spread() == BEGIN);
            break;
        case 4:
        case 5:
            CHECK(func[i].parameters().leading_self());
            CHECK(func[i].parameters().spread() == NO_SPREAD);
            break;
        case 6:
            CHECK(!func[i].parameters().leading_self());
            CHECK(func[i].parameters().spread() == END);
            break;
        case 7:
            CHECK(!func[i].parameters().leading_self());
            CHECK(func[i].parameters().spread() == BEGIN);
            break;
        case 8:
            CHECK(func[i].parameters().leading_self());
            CHECK(func[i].parameters().spread() == END);
            break;
        }
    }
    CHECK(!func[9].parameters().leading_self());
    CHECK(func[9].parameters().spread() == NO_SPREAD);
    CHECK(func[9].parameters().params().empty());
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
    CHECK(stats.size() == 4);
    auto opt1 = stats[0].options();
    CHECK(std::holds_alternative<WhileStatement>(opt1));
    auto while_stat = std::get_if<WhileStatement>(&opt1);
    CHECK(holds_alternative<BinaryOperation>(while_stat->exit_cond().options()));
    CHECK(while_stat->body().statements().size() == 1);
    CHECK(!while_stat->body().ret().has_value());
    auto opt2 = stats[1].options();
    CHECK(std::holds_alternative<WhileStatement>(opt2));
    while_stat = std::get_if<WhileStatement>(&opt2);
    CHECK(holds_alternative<BinaryOperation>(while_stat->exit_cond().options()));
    CHECK(while_stat->body().statements().empty());
    CHECK(!while_stat->body().ret().has_value());
    auto opt3 = stats[2].options();
    CHECK(std::holds_alternative<RepeatStatement>(opt3));
    auto repeat_stat = std::get_if<RepeatStatement>(&opt3);
    CHECK(holds_alternative<BinaryOperation>(repeat_stat->until_cond().options()));
    CHECK(repeat_stat->body().statements().empty());
    CHECK(!repeat_stat->body().ret().has_value());
    auto opt4 = stats[3].options();
    CHECK(std::holds_alternative<RepeatStatement>(opt4));
    repeat_stat = std::get_if<RepeatStatement>(&opt4);
    CHECK(holds_alternative<Identifier>(repeat_stat->until_cond().options()));
    CHECK(repeat_stat->body().statements().size() == 1);
    CHECK(repeat_stat->body().ret().has_value());
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
    CHECK(stats.size() == 3);
    std::vector<Return> returns;
    std::transform(stats.begin(), stats.end(), std::back_inserter(returns), [](Statement stat) {
        auto opt = stat.options();
        CHECK(std::holds_alternative<DoStatement>(opt));
        auto do_stat = *std::get_if<DoStatement>(&opt);
        CHECK(do_stat.body().ret().has_value());
        return do_stat.body().ret().value();
    });
    CHECK(returns[0].explist().size() == 4);
    for (uint i = 0; i < 4; i++) {
        CHECK(std::holds_alternative<Identifier>(returns[0].explist()[i].options()));
    }
    CHECK(returns[1].explist().empty());
    CHECK(returns[2].explist().size() == 1);
    CHECK(std::holds_alternative<Identifier>(returns[2].explist()[0].options()));
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
    CHECK(stats.size() == 6);
    // 1st statement
    auto opt1 = stats[0].options();
    CHECK(std::holds_alternative<VariableDeclaration>(opt1));
    auto var_dec1 = std::get_if<VariableDeclaration>(&opt1);
    CHECK(var_dec1->declarations().size() == 1);
    CHECK(var_dec1->declarators().size() == 1);
    CHECK(std::holds_alternative<Identifier>(var_dec1->declarators()[0].var()));
    CHECK(std::holds_alternative<Value>(var_dec1->declarations()[0].options()));
    // 2nd statement
    auto opt2 = stats[1].options();
    CHECK(std::holds_alternative<VariableDeclaration>(opt2));
    auto var_dec2 = std::get_if<VariableDeclaration>(&opt2);
    CHECK(var_dec2->declarations().size() == 3);
    CHECK(var_dec2->declarators().size() == 4);
    for (uint i = 0; i < 4; i++) {
        CHECK(std::holds_alternative<Identifier>(var_dec2->declarators()[i].var()));
    }
    CHECK(std::holds_alternative<BinaryOperation>(var_dec2->declarations()[0].options()));
    CHECK(std::holds_alternative<BinaryOperation>(var_dec2->declarations()[1].options()));
    CHECK(std::holds_alternative<Identifier>(var_dec2->declarations()[2].options()));
    // 3rd statement
    auto opt3 = stats[2].options();
    CHECK(std::holds_alternative<LocalVariableDeclaration>(opt3));
    auto var_dec3 = std::get_if<LocalVariableDeclaration>(&opt3);
    CHECK(var_dec3->declarations().empty());
    CHECK(var_dec3->declarators().size() == 1);
    CHECK(var_dec3->declarators()[0].str() == "e"s);
    // 4th statement
    auto opt4 = stats[3].options();
    CHECK(std::holds_alternative<LocalVariableDeclaration>(opt4));
    auto var_dec4 = std::get_if<LocalVariableDeclaration>(&opt4);
    CHECK(var_dec4->declarations().empty());
    CHECK(var_dec4->declarators().size() == 3);
    CHECK(var_dec4->declarators()[0].str() == "f"s);
    CHECK(var_dec4->declarators()[1].str() == "g"s);
    CHECK(var_dec4->declarators()[2].str() == "h"s);
    // 5th statement
    auto opt5 = stats[4].options();
    CHECK(std::holds_alternative<LocalVariableDeclaration>(opt5));
    auto var_dec5 = std::get_if<LocalVariableDeclaration>(&opt5);
    CHECK(var_dec5->declarations().size() == 2);
    CHECK(var_dec5->declarators().size() == 2);
    CHECK(std::holds_alternative<Value>(var_dec5->declarations()[0].options()));
    CHECK(std::holds_alternative<Value>(var_dec5->declarations()[1].options()));
    CHECK(var_dec5->declarators()[0].str() == "i"s);
    CHECK(var_dec5->declarators()[1].str() == "j"s);
    // 6th statement
    auto opt6 = stats[5].options();
    CHECK(std::holds_alternative<VariableDeclaration>(opt6));
    auto var_dec6 = std::get_if<VariableDeclaration>(&opt6);
    CHECK(var_dec6->declarators().size() == 1);
    auto declarator = var_dec6->declarators()[0];
    auto dec_opt1 = declarator.var();
    CHECK(std::holds_alternative<FieldExpression>(dec_opt1));
    auto fe1 = std::get_if<FieldExpression>(&dec_opt1);
    CHECK(fe1->property_id().str() == "field1");
    auto prefix1 = fe1->table_id().options();
    CHECK(std::holds_alternative<VariableDeclarator>(prefix1));
    auto dec2 = get_if<VariableDeclarator>(&prefix1);
    auto dec_opt2 = dec2->var();
    CHECK(holds_alternative<FieldExpression>(dec_opt2));
    auto fe2 = std::get_if<FieldExpression>(&dec_opt2);
    CHECK(fe2->property_id().str() == "table2");
    auto prefix2 = fe2->table_id().options();
    CHECK(std::holds_alternative<VariableDeclarator>(prefix2));
    auto dec3 = std::get_if<VariableDeclarator>(&prefix2);
    auto dec_opt3 = dec3->var();
    CHECK(holds_alternative<Identifier>(dec_opt3));
    auto table_id = std::get_if<Identifier>(&dec_opt3);
    CHECK(table_id->str() == "table1"s);
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
    CHECK(stats.size() == 1);
    auto opt1 = stats[0].options();
    CHECK(std::holds_alternative<Expression>(opt1));
    auto* exp1 = std::get_if<Expression>(&opt1);
    auto exp_opt1 = exp1->options();
    CHECK(std::holds_alternative<Table>(exp_opt1));
    auto table1 = std::get_if<Table>(&exp_opt1);
    auto fields = table1->fields();
    CHECK(fields.size() == 3);
    auto field1_opt = fields[0].content();
    CHECK(std::holds_alternative<ID_FIELD>(field1_opt));
    auto field1 = std::get_if<ID_FIELD>(&field1_opt);
    CHECK(field1->first.str() == "field1"s);
    CHECK(std::holds_alternative<Value>(field1->second.options()));
    auto content1_opt = field1->second.options();
    auto content1 = std::get_if<Value>(&content1_opt);
    CHECK(content1->is_string());
    auto raw = content1->raw();
    auto content1_str = get_if<String>(&raw);
    CHECK(content1_str->value == "name"s);
    auto field2_opt = fields[1].content();
    CHECK(std::holds_alternative<INDEX_FIELD>(field2_opt));
    auto field2 = std::get_if<INDEX_FIELD>(&field2_opt);
    CHECK(std::holds_alternative<BinaryOperation>(field2->first.options()));
    CHECK(std::holds_alternative<Table>(field2->second.options()));
    auto field3_opt = fields[2].content();
    CHECK(std::holds_alternative<Expression>(field3_opt));
    auto field3 = std::get_if<Expression>(&field3_opt);
    auto content3_opt = field3->options();
    CHECK(std::holds_alternative<FunctionDefinition>(content3_opt));
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
    CHECK(stats.size() == 5);
    std::vector<FunctionCall> func_calls;
    std::transform(stats.begin(), stats.end(), std::back_inserter(func_calls), [](Statement stat) {
        auto opt = stat.options();
        CHECK(std::holds_alternative<FunctionCall>(opt));
        return *std::get_if<FunctionCall>(&opt);
    });
    CHECK(!func_calls[0].method().has_value());
    CHECK(std::holds_alternative<VariableDeclarator>(func_calls[0].id().options()));
    auto opt1 = func_calls[0].id().options();
    auto name1 = std::get_if<VariableDeclarator>(&opt1);
    CHECK(holds_alternative<Identifier>(name1->var()));
    CHECK(func_calls[0].args().size() == 2);

    CHECK(!func_calls[1].method().has_value());
    CHECK(std::holds_alternative<VariableDeclarator>(func_calls[1].id().options()));
    auto opt2 = func_calls[1].id().options();
    auto name2 = std::get_if<VariableDeclarator>(&opt2);
    CHECK(holds_alternative<FieldExpression>(name2->var()));
    CHECK(func_calls[1].args().empty());

    CHECK(func_calls[2].method().has_value());
    CHECK(std::holds_alternative<VariableDeclarator>(func_calls[2].id().options()));
    auto opt3 = func_calls[2].id().options();
    auto name3 = std::get_if<VariableDeclarator>(&opt3);
    CHECK(holds_alternative<Identifier>(name3->var()));
    CHECK(func_calls[2].args().empty());

    CHECK(func_calls[3].args().size() == 1);
    CHECK(std::holds_alternative<Value>(func_calls[3].args()[0].options()));
    auto opt4 = func_calls[3].args()[0].options();
    auto str = std::get_if<Value>(&opt4);
    CHECK(str->is_string());

    CHECK(func_calls[4].args().size() == 1);
    CHECK(std::holds_alternative<Table>(func_calls[4].args()[0].options()));
    auto opt5 = func_calls[4].args()[0].options();
    auto table = std::get_if<Table>(&opt5);
    CHECK(table->fields().size() == 4);
    for (uint i = 0; i < 4; i++) {
        CHECK(std::holds_alternative<Expression>(table->fields()[i].content()));
    }
}
} // namespace minilua::details