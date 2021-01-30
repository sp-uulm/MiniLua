#include "details/ast.hpp"
#include "tree_sitter/tree_sitter.hpp"
#include <catch2/catch.hpp>
#include <iostream>
#include <type_traits>

namespace minilua::details::ast {

TEST_CASE("statements", "[tree-sitter]") {
    ts::Parser parser;
    std::string source = "i,t,l = 5\n"
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
                         "foo(i,k,z)\n"
                         "function (a,b)\n"
                         "print(a .. b)\n"
                         "end\n"
                         "local function foo (f,o,oo)\n"
                         "return f,o*oo\n"
                         "end\n";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    CHECK(!body.return_statement().has_value());
    vector<Statement> statement = body.statements();
    CHECK(statement.size() == 14);
    long unsigned int statement_count = statement.size();
    // this loop tests if each statement got parsed to the right Class
    for (long unsigned int i = 0; i < statement_count-1; i++) {
        CHECK(statement.at(i).options().index() == i);
    }
    CHECK(std::holds_alternative<FunctionStatement>(statement.at(statement_count-1).options()));
}
TEST_CASE("expressions", "[tree-sitter]") {
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
    CHECK(!body.return_statement().has_value());
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
    CHECK(std::holds_alternative<Spread>(spread));
    auto next = exps[1].options();
    CHECK(std::holds_alternative<Identifier>(next));
    auto func_def = exps[2].options();
    CHECK(std::holds_alternative<FunctionDefinition>(func_def));
    auto table = exps[3].options();
    CHECK(std::holds_alternative<Table>(table));
    vector<BinaryOperation> bin_ops;
    bin_ops.reserve(25);
    std::transform(
        exps.begin() + 4, exps.begin() + 25, std::back_inserter(bin_ops), [](Expression exp) {
            auto opt = exp.options();
            return *get_if<BinaryOperation>(&opt);
        });
    for (uint i = 0; i < bin_ops.size(); i++) {
        CHECK(
            bin_ops[i].binary_operator() ==
            (BinOpEnum)i); // Bin Operations are in the same sequence as in the BinOpEnum
    }

    auto nil = exps[25].options();
    CHECK(holds_alternative<Literal>(nil));
    auto* temp = get_if<Literal>(&nil);
    CHECK(temp->type() == LiteralType::NIL);
    auto _true = exps[26].options();
    CHECK(holds_alternative<Literal>(_true));
    temp = get_if<Literal>(&_true);
    CHECK(temp->type() == LiteralType::TRUE);
    auto _false = exps[27].options();
    CHECK(holds_alternative<Literal>(_false));
    temp = get_if<Literal>(&_false);
    CHECK(temp->type() == LiteralType::FALSE);
    auto id = exps[28].options();
    CHECK(holds_alternative<Identifier>(id));
    Identifier temp2 = *get_if<Identifier>(&id);
    CHECK(temp2.string() == "id"s);
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
        CHECK(un_op[j].unary_operator() == (UnOpEnum)j);
    }
}

TEST_CASE("do_statements", "[tree-sitter]") {
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
    CHECK(!body.return_statement().has_value());
    std::vector<DoStatement> dos;
    std::transform(stats.begin(), stats.end(), std::back_inserter(dos), [](Statement stat) {
        auto opt = stat.options();
        CHECK(std::holds_alternative<DoStatement>(opt));
        return *std::get_if<DoStatement>(&opt);
    });
    CHECK(dos.size() == 5);
    CHECK(dos[0].body().statements().empty());
    CHECK(!dos[0].body().return_statement().has_value());
    CHECK(dos[1].body().statements().size() == 1);
    CHECK(!dos[1].body().return_statement().has_value());
    CHECK(dos[2].body().statements().size() == 2);
    CHECK(!dos[2].body().return_statement().has_value());
    CHECK(dos[3].body().statements().empty());
    CHECK(dos[3].body().return_statement().has_value());
    CHECK(dos[4].body().statements().size() == 1);
    CHECK(dos[4].body().return_statement().has_value());
}
TEST_CASE("if_statements", "[tree-sitter]") {
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
    CHECK(holds_alternative<BinaryOperation>(ifs[0].condition().options()));
    CHECK(ifs[0].body().statements().empty());
    CHECK(!ifs[0].body().return_statement().has_value());
    CHECK(ifs[0].elseifs().size() == 2);
    CHECK(holds_alternative<Identifier>(ifs[0].elseifs()[0].condition().options()));
    CHECK(ifs[0].elseifs()[0].body().statements().size() == 1);
    CHECK(!ifs[0].elseifs()[0].body().return_statement().has_value());
    CHECK(holds_alternative<Literal>(ifs[0].elseifs()[1].condition().options()));
    CHECK(ifs[0].elseifs()[1].body().statements().size() == 2);
    CHECK(!ifs[0].elseifs()[1].body().return_statement().has_value());
    CHECK(ifs[0].else_statement().has_value());
    CHECK(ifs[0].else_statement().value().body().statements().empty());
    CHECK(ifs[0].else_statement().value().body().return_statement().has_value());
    CHECK(std::holds_alternative<Literal>(
        ifs[0].else_statement().value().body().return_statement().value().exp_list()[0].options()));
    CHECK(ifs[1].body().statements().empty());
    CHECK(ifs[1].body().return_statement().has_value());
    CHECK(ifs[1].elseifs().empty());
    CHECK(!ifs[1].else_statement().has_value());
    CHECK(holds_alternative<Identifier>(ifs[1].condition().options()));
    CHECK(holds_alternative<Identifier>(ifs[2].condition().options()));
    CHECK(ifs[2].body().statements().size() == 1);
    CHECK(!ifs[2].body().return_statement().has_value());
    CHECK(ifs[2].elseifs().empty());
    CHECK(ifs[2].else_statement().has_value());
    CHECK(ifs[2].else_statement().value().body().statements().size() == 1);
    CHECK(!ifs[2].else_statement().value().body().return_statement().has_value());
    CHECK(holds_alternative<Literal>(ifs[3].condition().options()));
    CHECK(ifs[3].body().statements().empty());
    CHECK(!ifs[3].body().return_statement().has_value());
    CHECK(ifs[3].elseifs().empty());
    CHECK(ifs[3].else_statement().has_value());
    CHECK(ifs[3].else_statement().value().body().statements().size() == 1);
    CHECK(!ifs[3].else_statement().value().body().return_statement().has_value());
    CHECK(holds_alternative<Identifier>(ifs[4].condition().options()));
    CHECK(ifs[4].body().statements().size() == 1);
    CHECK(!ifs[4].body().return_statement().has_value());
    CHECK(ifs[4].elseifs().size() == 2);
    CHECK(!ifs[4].else_statement().has_value());
    CHECK(ifs[4].elseifs()[0].body().statements().size() == 1);
    CHECK(!ifs[4].elseifs()[0].body().return_statement().has_value());
    CHECK(ifs[4].elseifs()[1].body().statements().size() == 2);
    CHECK(!ifs[4].elseifs()[1].body().return_statement().has_value());
    CHECK(ifs[5].body().statements().empty());
    CHECK(!ifs[5].body().return_statement().has_value());
    CHECK(ifs[5].elseifs().empty());
    CHECK(!ifs[5].else_statement().has_value());
}
TEST_CASE("for_statements", "[tree-sitter]") {
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
    CHECK(!body.return_statement().has_value());
    std::vector<ForStatement> fors;
    std::transform(stats.begin(), stats.end(), std::back_inserter(fors), [](Statement stat) {
        auto opt = stat.options();
        CHECK(std::holds_alternative<ForStatement>(opt));
        return *std::get_if<ForStatement>(&opt);
    });
    CHECK(fors.size() == 3);
    // 1st loop
    CHECK(fors[0].body().statements().size() == 1);
    CHECK(!fors[0].body().return_statement().has_value());
    CHECK(fors[0].loop_expression().variable().string() == "i"s);
    auto start1_opt = fors[0].loop_expression().start().options();
    CHECK(holds_alternative<Literal>(start1_opt));
    auto* start1 = get_if<Literal>(&start1_opt);
    CHECK(start1->type() == LiteralType::NUMBER);
    CHECK(start1->content() == "1"s);
    auto end1_opt = fors[0].loop_expression().end().options();
    CHECK(holds_alternative<Literal>(end1_opt));
    auto* end1 = get_if<Literal>(&end1_opt);
    CHECK(end1->type() == LiteralType::NUMBER);
    CHECK(end1->content() == "2"s);
    CHECK(!fors[0].loop_expression().step().has_value());
    // 2nd loop just to check if the empty body works fine here
    CHECK(fors[1].body().statements().empty());
    // 3rd loop
    CHECK(fors[2].body().statements().size() == 3);
    CHECK(!fors[2].body().return_statement().has_value());
    // checking the loopexpression
    CHECK(fors[2].loop_expression().variable().string() == "c"s);
    auto start3_opt = fors[2].loop_expression().start().options();
    CHECK(holds_alternative<Identifier>(start3_opt));
    auto start3 = get_if<Identifier>(&start3_opt);
    CHECK(start3->string() == "a"s);
    CHECK(fors[2].loop_expression().step().has_value());
    auto step3_opt = fors[2].loop_expression().step()->options();
    CHECK(holds_alternative<Identifier>(step3_opt));
    auto step3 = get_if<Identifier>(&step3_opt);
    CHECK(step3->string() == "b"s);
    auto end3_opt = fors[2].loop_expression().end().options();
    CHECK(holds_alternative<Literal>(end3_opt));
    auto end3 = get_if<Literal>(&end3_opt);
    CHECK(end3->type() == LiteralType::NUMBER);
    CHECK(end3->content() == "42"s);
}
TEST_CASE("for_in_statements", "[tree-sitter]") {
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
    CHECK(!body.return_statement().has_value());
    std::vector<ForInStatement> fors;
    std::transform(stats.begin(), stats.end(), std::back_inserter(fors), [](Statement stat) {
        auto opt = stat.options();
        CHECK(std::holds_alternative<ForInStatement>(opt));
        return *std::get_if<ForInStatement>(&opt);
    });
    CHECK(fors.size() == 2);
    CHECK(fors[0].loop_expression().loop_vars().size() == 2);
    CHECK(fors[0].loop_expression().loop_exps().size() == 3);
    CHECK(fors[1].loop_expression().loop_vars().size() == 5);
    CHECK(fors[1].loop_expression().loop_exps().size() == 1);
}
TEST_CASE("function_statements", "[tree-sitter]") {
    ts::Parser parser;
    std::string source = "function foo (a,b,c)\n"
                         "  1+1\n"
                         "  return 3+3,2+2,a,b,c,d \n"
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
    CHECK(!body.return_statement().has_value());
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
    CHECK(func[0].body().return_statement().has_value());
    auto ret = func[0].body().return_statement().value();
    CHECK(ret.exp_list().size()==6);
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
                [](Identifier id) { return id.string(); });
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
TEST_CASE("while_and_repeat_statements", "[tree-sitter]") {
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
    CHECK(holds_alternative<BinaryOperation>(while_stat->repeat_conditon().options()));
    CHECK(while_stat->body().statements().size() == 1);
    CHECK(!while_stat->body().return_statement().has_value());
    auto opt2 = stats[1].options();
    CHECK(std::holds_alternative<WhileStatement>(opt2));
    while_stat = std::get_if<WhileStatement>(&opt2);
    CHECK(holds_alternative<BinaryOperation>(while_stat->repeat_conditon().options()));
    CHECK(while_stat->body().statements().empty());
    CHECK(!while_stat->body().return_statement().has_value());
    auto opt3 = stats[2].options();
    CHECK(std::holds_alternative<RepeatStatement>(opt3));
    auto repeat_stat = std::get_if<RepeatStatement>(&opt3);
    CHECK(holds_alternative<BinaryOperation>(repeat_stat->repeat_condition().options()));
    CHECK(repeat_stat->body().statements().empty());
    CHECK(!repeat_stat->body().return_statement().has_value());
    auto opt4 = stats[3].options();
    CHECK(std::holds_alternative<RepeatStatement>(opt4));
    repeat_stat = std::get_if<RepeatStatement>(&opt4);
    CHECK(holds_alternative<Identifier>(repeat_stat->repeat_condition().options()));
    CHECK(repeat_stat->body().statements().size() == 1);
    CHECK(repeat_stat->body().return_statement().has_value());
}
TEST_CASE("return_statements", "[tree-sitter]") {
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
        CHECK(do_stat.body().statements().empty());
        CHECK(do_stat.body().return_statement().has_value());
        return do_stat.body().return_statement().value();
    });
    CHECK(returns[0].exp_list().size() == 4);
    for (uint i = 0; i < 4; i++) {
        CHECK(std::holds_alternative<Identifier>(returns[0].exp_list()[i].options()));
    }
    CHECK(returns[1].exp_list().empty());
    CHECK(returns[2].exp_list().size() == 1);
    CHECK(std::holds_alternative<Identifier>(returns[2].exp_list()[0].options()));
}
TEST_CASE("var_dec_statements", "[tree-sitter]") {
    ts::Parser parser;
    std::string source = "a = 1\n"
                         "a,b,c,d = 1+2,5+7,a\n"
                         "local e\n"
                         "local f,g,h\n"
                         "local i,j = 42,96\n"
                         "\n"
                         "table1.table2.field1 = function() print(2) end\n"
                         "table1[table2] = 2\n";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    Body body = prog.body();
    auto stats = body.statements();
    CHECK(stats.size() == 7);
    // 1st statement
    auto opt1 = stats[0].options();
    CHECK(std::holds_alternative<VariableDeclaration>(opt1));
    auto var_dec1 = std::get_if<VariableDeclaration>(&opt1);
    CHECK(!var_dec1->local());
    CHECK(var_dec1->declarations().size() == 1);
    CHECK(var_dec1->declarators().size() == 1);
    CHECK(std::holds_alternative<Identifier>(var_dec1->declarators()[0].options()));
    CHECK(std::holds_alternative<Literal>(var_dec1->declarations()[0].options()));
    // 2nd statement
    auto opt2 = stats[1].options();
    CHECK(std::holds_alternative<VariableDeclaration>(opt2));
    auto var_dec2 = std::get_if<VariableDeclaration>(&opt2);
    CHECK(!var_dec2->local());
    CHECK(var_dec2->declarations().size() == 3);
    CHECK(var_dec2->declarators().size() == 4);
    for (uint i = 0; i < 4; i++) {
        CHECK(std::holds_alternative<Identifier>(var_dec2->declarators()[i].options()));
    }
    CHECK(std::holds_alternative<BinaryOperation>(var_dec2->declarations()[0].options()));
    CHECK(std::holds_alternative<BinaryOperation>(var_dec2->declarations()[1].options()));
    CHECK(std::holds_alternative<Identifier>(var_dec2->declarations()[2].options()));
    // 3rd statement
    auto opt3 = stats[2].options();
    CHECK(std::holds_alternative<VariableDeclaration>(opt3));
    auto var_dec3 = std::get_if<VariableDeclaration>(&opt3);
    CHECK(var_dec3->local());
    CHECK(var_dec3->declarations().empty());
    CHECK(var_dec3->declarators().size() == 1);
    CHECK(std::holds_alternative<Identifier>(var_dec3->declarators()[0].options()));
    auto id1_opt = var_dec3->declarators()[0].options();
    auto id1 = std::get_if<Identifier>(&id1_opt);
    CHECK(id1->string() == "e"s);
    // 4th statement
    auto opt4 = stats[3].options();
    CHECK(std::holds_alternative<VariableDeclaration>(opt4));
    auto var_dec4 = std::get_if<VariableDeclaration>(&opt4);
    CHECK(var_dec4->local());
    CHECK(var_dec4->declarations().empty());
    CHECK(var_dec4->declarators().size() == 3);
    CHECK(std::holds_alternative<Identifier>(var_dec4->declarators()[0].options()));
    auto id2_opt = var_dec4->declarators()[0].options();
    auto id2 = std::get_if<Identifier>(&id2_opt);
    CHECK(id2->string() == "f"s);
    CHECK(std::holds_alternative<Identifier>(var_dec4->declarators()[1].options()));
    auto id3_opt = var_dec4->declarators()[1].options();
    auto id3 = std::get_if<Identifier>(&id3_opt);
    CHECK(id3->string() == "g"s);
    CHECK(std::holds_alternative<Identifier>(var_dec4->declarators()[2].options()));
    auto id4_opt = var_dec4->declarators()[2].options();
    auto id4 = std::get_if<Identifier>(&id4_opt);
    CHECK(id4->string() == "h"s);
    // 5th statement
    auto opt5 = stats[4].options();
    CHECK(std::holds_alternative<VariableDeclaration>(opt5));
    auto var_dec5 = std::get_if<VariableDeclaration>(&opt5);
    CHECK(var_dec5->local());
    CHECK(var_dec5->declarations().size() == 2);
    CHECK(var_dec5->declarators().size() == 2);
    CHECK(std::holds_alternative<Literal>(var_dec5->declarations()[0].options()));
    CHECK(std::holds_alternative<Literal>(var_dec5->declarations()[1].options()));
    CHECK(std::holds_alternative<Identifier>(var_dec5->declarators()[0].options()));
    auto id5_opt = var_dec5->declarators()[0].options();
    auto id5 = std::get_if<Identifier>(&id5_opt);
    CHECK(id5->string() == "i"s);
    CHECK(std::holds_alternative<Identifier>(var_dec5->declarators()[1].options()));
    auto id6_opt = var_dec5->declarators()[1].options();
    auto id6 = std::get_if<Identifier>(&id6_opt);
    CHECK(id6->string() == "j"s);
    // 6th statement
    auto opt6 = stats[5].options();
    CHECK(std::holds_alternative<VariableDeclaration>(opt6));
    auto var_dec6 = std::get_if<VariableDeclaration>(&opt6);
    CHECK(var_dec6->declarators().size() == 1);
    auto declarator = var_dec6->declarators()[0];
    auto dec_opt1 = declarator.options();
    CHECK(std::holds_alternative<FieldExpression>(dec_opt1));
    auto fe1 = std::get_if<FieldExpression>(&dec_opt1);
    CHECK(fe1->property_id().string() == "field1");
    auto prefix1 = fe1->table_id().options();
    CHECK(std::holds_alternative<VariableDeclarator>(prefix1));
    auto dec2 = get_if<VariableDeclarator>(&prefix1);
    auto dec_opt2 = dec2->options();
    CHECK(holds_alternative<FieldExpression>(dec_opt2));
    auto fe2 = std::get_if<FieldExpression>(&dec_opt2);
    CHECK(fe2->property_id().string() == "table2");
    auto prefix2 = fe2->table_id().options();
    CHECK(std::holds_alternative<VariableDeclarator>(prefix2));
    auto dec3 = std::get_if<VariableDeclarator>(&prefix2);
    auto dec_opt3 = dec3->options();
    CHECK(holds_alternative<Identifier>(dec_opt3));
    auto table_id = std::get_if<Identifier>(&dec_opt3);
    CHECK(table_id->string() == "table1"s);
    //7th statement
    auto opt7 = stats[6].options();
    CHECK(std::holds_alternative<VariableDeclaration>(opt7));
    auto var_dec7 = std::get_if<VariableDeclaration>(&opt7);
    CHECK(var_dec7->declarations().size() == 1);
    CHECK(var_dec7->declarators().size() == 1);
    auto ti_opt1 = var_dec7->declarators()[0].options();
    CHECK(std::holds_alternative<TableIndex>(ti_opt1));
    auto ti1 = std::get_if<TableIndex>(&ti_opt1);
    auto pref1 = ti1->table().options();
    CHECK(std::holds_alternative<VariableDeclarator>(pref1));
    auto pref_dec1 = std::get_if<VariableDeclarator>(&pref1);
    auto pref_opt1 = pref_dec1->options();
    CHECK(std::holds_alternative<Identifier>(pref_opt1));
    auto id7 = std::get_if<Identifier>(&pref_opt1);
    CHECK(id7->string()=="table1");
    auto index1_opt = ti1->index().options();
    CHECK(std::holds_alternative<Identifier>(index1_opt));
    auto index1 = std::get_if<Identifier>(&index1_opt);
    CHECK(index1->string()=="table2");
}
TEST_CASE("table_statements", "[tree-sitter]") {
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
    CHECK(std::holds_alternative<IdentifierField>(field1_opt));
    auto field1 = std::get_if<IdentifierField>(&field1_opt);
    CHECK(field1->first.string() == "field1"s);
    CHECK(std::holds_alternative<Literal>(field1->second.options()));
    auto content1_opt = field1->second.options();
    auto content1 = std::get_if<Literal>(&content1_opt);
    CHECK(content1->type() == LiteralType::STRING);
    CHECK(content1->content() == "\"name\""s);
    auto field2_opt = fields[1].content();
    CHECK(std::holds_alternative<IndexField>(field2_opt));
    auto field2 = std::get_if<IndexField>(&field2_opt);
    CHECK(std::holds_alternative<BinaryOperation>(field2->first.options()));
    CHECK(std::holds_alternative<Table>(field2->second.options()));
    auto field3_opt = fields[2].content();
    CHECK(std::holds_alternative<Expression>(field3_opt));
    auto field3 = std::get_if<Expression>(&field3_opt);
    auto content3_opt = field3->options();
    CHECK(std::holds_alternative<FunctionDefinition>(content3_opt));
    auto func_def = std::get_if<FunctionDefinition>(&content3_opt);
    CHECK(func_def->body().statements().empty());
    CHECK(func_def->body().return_statement().has_value());
}
TEST_CASE("function_calls", "[tree-sitter]") {
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
    CHECK(holds_alternative<Identifier>(name1->options()));
    CHECK(func_calls[0].args().size() == 2);

    CHECK(!func_calls[1].method().has_value());
    CHECK(std::holds_alternative<VariableDeclarator>(func_calls[1].id().options()));
    auto opt2 = func_calls[1].id().options();
    auto name2 = std::get_if<VariableDeclarator>(&opt2);
    CHECK(holds_alternative<FieldExpression>(name2->options()));
    CHECK(func_calls[1].args().empty());

    CHECK(func_calls[2].method().has_value());
    CHECK(std::holds_alternative<VariableDeclarator>(func_calls[2].id().options()));
    auto opt3 = func_calls[2].id().options();
    auto name3 = std::get_if<VariableDeclarator>(&opt3);
    CHECK(holds_alternative<Identifier>(name3->options()));
    CHECK(func_calls[2].args().empty());

    CHECK(func_calls[3].args().size() == 1);
    CHECK(std::holds_alternative<Literal>(func_calls[3].args()[0].options()));
    auto opt4 = func_calls[3].args()[0].options();
    auto str = std::get_if<Literal>(&opt4);
    CHECK(str->type() == LiteralType::STRING);
    CHECK(str->content() == "\"abc\"");
    CHECK(func_calls[4].args().size() == 1);
    CHECK(std::holds_alternative<Table>(func_calls[4].args()[0].options()));
    auto opt5 = func_calls[4].args()[0].options();
    auto table = std::get_if<Table>(&opt5);
    CHECK(table->fields().size() == 4);
    for (uint i = 0; i < 4; i++) {
        CHECK(std::holds_alternative<Expression>(table->fields()[i].content()));
    }
}

TEST_CASE("comment_test", "[tree-sitter]") {
    /**
     * only unary- and binary- operations are checked because every other class only uses
     * named nodes and comments are anonymus nodes
     */
    ts::Parser parser;
    std::string source = "i,j = --[[abc]]#--[[alfs]]table,"
                         " --[[a comment]] a --[[a dumb comment]]+--[[abc]]b\n"
                         "--[[asd]]";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();
    auto prog = Program(root);
    auto body = prog.body();
    auto stat = body.statements()[0].options();
    CHECK(std::holds_alternative<VariableDeclaration>(stat));
    auto var_dec = std::get_if<VariableDeclaration>(&stat);
    auto exps = var_dec->declarations();
    CHECK(std::holds_alternative<UnaryOperation>(exps[0].options()));
    auto exp1_opt = exps[0].options();
    auto un1 = std::get_if<UnaryOperation>(&exp1_opt);
    CHECK(un1->unary_operator()==UnOpEnum::LEN);
    CHECK(std::holds_alternative<Identifier>(un1->expression().options()));
    auto operand1_opt = un1->expression().options();
    auto operand1 = std::get_if<Identifier>(&operand1_opt);
    CHECK(operand1->string()=="table"s);

    CHECK(std::holds_alternative<BinaryOperation>(exps[1].options()));
    auto exp2_opt = exps[1].options();
    auto bin1 = std::get_if<BinaryOperation>(&exp2_opt);
    CHECK(bin1->binary_operator() == BinOpEnum::ADD);
    CHECK(std::holds_alternative<Identifier>(bin1->left().options()));
    auto operand_left_opt = bin1->left().options();
    auto operand_left = std::get_if<Identifier>(&operand_left_opt);
    CHECK(operand_left->string()=="a"s);
    CHECK(std::holds_alternative<Identifier>(bin1->right().options()));
    auto operand_right_opt = bin1->right().options();
    auto operand_right = std::get_if<Identifier>(&operand_right_opt);
    CHECK(operand_right->string()=="b"s);
}

} // namespace minilua::details::ast
