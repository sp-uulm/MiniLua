#include <catch2/catch.hpp>
#include <cstring>
#include <iostream>
#include <type_traits>
#include "tree_sitter/tree_sitter.hpp"
#include "MiniLua/tree_sitter_ast.hpp"
using namespace std::string_literals;

std::basic_string<char> all_children(ts::Node node){
    std::vector<ts::Node> children = node.children();
    std::basic_string<char> res;
    std::vector<ts::Node>::iterator it;
    for(it = children.begin();it!=children.end();it++){
        res += it->as_s_expr();
        res += "\n\n";
    }
    return res;
}

std::basic_string<char> all_named_children(ts::Node node){
    std::vector<ts::Node> children = node.named_children();
    std::basic_string<char> res;
    std::vector<ts::Node>::iterator it;
    for(it = children.begin();it!=children.end();it++){
        res += it->as_s_expr();
        res += "\n\n";
    }
    return res;
}

/*TEST_CASE("Print3", "[tree-sitter][!hide]") {
    ts::Parser parser;

    std::string source = "local teams = {\n"
                         "    [\"teamA\"] = 12,\n"
                         "    [\"teamB\"] = 15\n"
                         "}\n"
                         "\n"
                         "print(teams[\"teamA\"]) -- 12\n"
                         "\n"
                         "for key,value in pairs(teams) do\n"
                         "  print(key .. \":\" .. value)\n"
                         "end";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();

    INFO(all_named_children(root));
    FAIL();
}

TEST_CASE("iftest", "[tree-sitter][!hide]") {
    ts::Parser parser;

    std::string source = "if 1<4 then\n1+1\n2+2\n3+3\nelseif false then\n2+2\nelseif true then\n1+1\nelse 3+3\nend";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();

    INFO(all_named_children(root.named_child(0).value()));
    FAIL();
}
TEST_CASE("functiontest", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "function tester(a,b,c)\nreturn a+b+c\nend\nlocal function t(a,b,c)\nreturn a,b,c\nend";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();

    INFO(all_named_children(root));
    FAIL();
}
TEST_CASE("variabledectest", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "i\nlocal b = 5\n i = b";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();

    INFO(root);
    FAIL();
}

TEST_CASE("tablesandstuff", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "local pet = {\n"
                         "age = 0,\n"
                         "hungry = function (self) print(self.type .. \" is hungry!\") end\n"
                         "}\n"
                         "pet.type  = \"\"\n"
                         "\n"
                         "\n"
                         "local dog = pet\n"
                         "dog.type = 3\n"
                         "\n"
                         "print(dog.type)\n"
                         "dog:hungry()";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();

    INFO(all_children(root));
    FAIL();
}
TEST_CASE("function", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "function n()\n"
                         " print(2)\n"
                         "end\n"
                         "\n"
                         "n()";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();

    INFO(all_children(root));
    FAIL();
}
TEST_CASE("Print2", "[tree-sitter][!hide]") {



    ts::Parser parser;

    std::string source = "if 2<6 then\ni=1+2\n1+2\n1+2\n1+2\nelseif false then\n1+2\nelse\n\"w\" .. 2\nend";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();

    INFO(all_named_children(root.child(0).value()));
    FAIL();
}

TEST_CASE("variables", "[tree-sitter][!hide]") {
    ts::Parser parser;
    std::string source = "a = {t1 = 2;\n"
                         "     t2 =  4}\n"
                         "a.t2 = 5+3\n"
                         "c =  a     [\"t2\"]\n"
                         "function (a)\n"
                         "2+2\nend\ndoa \"a\"";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();

    INFO(all_children(root));
    FAIL();
}

*/
TEST_CASE("Print42", "[tree-sitter][!hide]") {
    ts::Parser parser;

    std::string source = "for i=1,5 do\n"
                         "print(i)\n"
                         "end";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node().child(0).value();

    INFO(root.child_count());
    FAIL();
}
