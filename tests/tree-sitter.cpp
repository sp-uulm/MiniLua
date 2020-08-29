#include <catch2/catch.hpp>
#include <cstring>
#include <tree_sitter/api.h>

#include "tree-sitter/tree-sitter.hpp"

using namespace std::string_literals;

TEST_CASE("Tree-Sitter-Wrapper", "[parse]") {
    SECTION("Simple addition") {
        ts::Parser parser;

        std::string source_code = "1 + 2";
        ts::Tree tree = parser.parse_string(source_code);

        ts::Node root_node = tree.get_root_node();
        INFO(root_node.as_string());
        CHECK(root_node.get_type() == "program"s);

        ts::Node expr_node = root_node.get_child(0);
        CHECK(expr_node.get_type() == "expression"s);

        ts::Node bin_op_node = expr_node.get_named_child(0);
        CHECK(bin_op_node.get_type() == "binary_operation"s);
        CHECK(bin_op_node.get_named_child_count() == 2);
        CHECK(bin_op_node.get_start_byte() == 0);
        CHECK(bin_op_node.get_end_byte() == 5);
        CHECK(bin_op_node.get_start_point() == ts::Point{.row = 0, .column = 0});
        CHECK(bin_op_node.get_end_point() == ts::Point{.row = 0, .column = 5});

        ts::Node number_1_node = bin_op_node.get_named_child(0);
        CHECK(number_1_node.get_type() == "number"s);
        CHECK(number_1_node.get_start_byte() == 0);
        CHECK(number_1_node.get_end_byte() == 1);
        CHECK(number_1_node.get_start_point() == ts::Point{.row = 0, .column = 0});
        CHECK(number_1_node.get_end_point() == ts::Point{.row = 0, .column = 1});

        ts::Node number_2_node = bin_op_node.get_named_child(1);
        CHECK(number_2_node.get_type() == "number"s);
        CHECK(number_2_node.get_start_byte() == 4);
        CHECK(number_2_node.get_end_byte() == 5);
        CHECK(number_2_node.get_start_point() == ts::Point{.row = 0, .column = 4});
        CHECK(number_2_node.get_end_point() == ts::Point{.row = 0, .column = 5});
    }
    SECTION("If example") {
        ts::Parser parser;

        std::string source_code = R"-(if true then
    print(1)
    print(2)
else
    print(3)
    print(4)
end
)-";
        ts::Tree tree = parser.parse_string(source_code);

        ts::Node root_node = tree.get_root_node();
        INFO(root_node.as_string());
        CHECK(root_node.get_type() == "program"s);

        ts::Node if_stmt = root_node.get_child(0);
        CHECK(if_stmt.get_type() == "if_statement"s);
        CHECK(if_stmt.get_named_child_count() == 4);

        ts::Node condition = if_stmt.get_named_child(0);
        CHECK(condition.get_type() == "condition_expression"s);
        CHECK(condition.get_named_child_count() == 1);

        ts::Node true_lit = condition.get_named_child(0);
        CHECK(true_lit.get_type() == "true"s);

        {
            ts::Node call1 = if_stmt.get_named_child(1);
            CHECK(call1.get_type() == "function_call"s);
            CHECK(call1.get_start_byte() == 17);
            CHECK(call1.get_end_byte() == 25);
            CHECK(call1.get_start_point() == ts::Point{.row = 1, .column = 4});
            CHECK(call1.get_end_point() == ts::Point{.row = 1, .column = 12});
            CHECK(call1.get_text(source_code) == "print(1)"s);

            ts::Node call1_ident = call1.get_named_child(0);
            CHECK(call1_ident.get_type() == "identifier"s);
            CHECK(call1_ident.get_start_byte() == 17);
            CHECK(call1_ident.get_end_byte() == 22);
            CHECK(call1_ident.get_start_point() == ts::Point{.row = 1, .column = 4});
            CHECK(call1_ident.get_end_point() == ts::Point{.row = 1, .column = 9});
            CHECK(call1_ident.get_text(source_code) == "print"s);

            ts::Node call1_args = call1.get_named_child(1);
            CHECK(call1_args.get_type() == "arguments"s);
            CHECK(call1_args.get_named_child_count() == 1);

            ts::Node call1_arg1 = call1_args.get_named_child(0);
            CHECK(call1_arg1.get_type() == "number"s);
            CHECK(call1_arg1.get_start_byte() == 23);
            CHECK(call1_arg1.get_end_byte() == 24);
            CHECK(call1_arg1.get_start_point() == ts::Point{.row = 1, .column = 10});
            CHECK(call1_arg1.get_end_point() == ts::Point{.row = 1, .column = 11});
            CHECK(call1_arg1.get_text(source_code) == "1"s);
        }

        {
            ts::Node call2 = if_stmt.get_named_child(2);
            CHECK(call2.get_type() == "function_call"s);
            CHECK(call2.get_start_byte() == 30);
            CHECK(call2.get_end_byte() == 38);
            CHECK(call2.get_start_point() == ts::Point{.row = 2, .column = 4});
            CHECK(call2.get_end_point() == ts::Point{.row = 2, .column = 12});
            CHECK(call2.get_text(source_code) == "print(2)"s);

            ts::Node call2_ident = call2.get_named_child(0);
            CHECK(call2_ident.get_type() == "identifier"s);
            CHECK(call2_ident.get_start_byte() == 30);
            CHECK(call2_ident.get_end_byte() == 35);
            CHECK(call2_ident.get_start_point() == ts::Point{.row = 2, .column = 4});
            CHECK(call2_ident.get_end_point() == ts::Point{.row = 2, .column = 9});
            CHECK(call2_ident.get_text(source_code) == "print"s);

            ts::Node call2_args = call2.get_named_child(1);
            CHECK(call2_args.get_type() == "arguments"s);
            CHECK(call2_args.get_named_child_count() == 1);

            ts::Node call2_arg1 = call2_args.get_named_child(0);
            CHECK(call2_arg1.get_type() == "number"s);
            CHECK(call2_arg1.get_start_byte() == 36);
            CHECK(call2_arg1.get_end_byte() == 37);
            CHECK(call2_arg1.get_start_point() == ts::Point{.row = 2, .column = 10});
            CHECK(call2_arg1.get_end_point() == ts::Point{.row = 2, .column = 11});
            CHECK(call2_arg1.get_text(source_code) == "2"s);
        }

        {
            ts::Node else_branch = if_stmt.get_named_child(3);
            CHECK(else_branch.get_type() == "else"s);
            CHECK(else_branch.get_start_byte() == 39);
            CHECK(else_branch.get_end_byte() == 69);
            CHECK(else_branch.get_start_point() == ts::Point{.row = 3, .column = 0});
            CHECK(else_branch.get_end_point() == ts::Point{.row = 5, .column = 12});
            CHECK(else_branch.get_named_child_count() == 2);

            {
                ts::Node call3 = else_branch.get_named_child(0);
                CHECK(call3.get_type() == "function_call"s);
                CHECK(call3.get_start_byte() == 48);
                CHECK(call3.get_end_byte() == 56);
                CHECK(call3.get_start_point() == ts::Point{.row = 4, .column = 4});
                CHECK(call3.get_end_point() == ts::Point{.row = 4, .column = 12});
                CHECK(call3.get_text(source_code) == "print(3)"s);

                ts::Node call3_ident = call3.get_named_child(0);
                CHECK(call3_ident.get_type() == "identifier"s);
                CHECK(call3_ident.get_start_byte() == 48);
                CHECK(call3_ident.get_end_byte() == 53);
                CHECK(call3_ident.get_start_point() == ts::Point{.row = 4, .column = 4});
                CHECK(call3_ident.get_end_point() == ts::Point{.row = 4, .column = 9});
                CHECK(call3_ident.get_text(source_code) == "print"s);

                ts::Node call3_args = call3.get_named_child(1);
                CHECK(call3_args.get_type() == "arguments"s);
                CHECK(call3_args.get_named_child_count() == 1);

                ts::Node call3_arg1 = call3_args.get_named_child(0);
                CHECK(call3_arg1.get_type() == "number"s);
                CHECK(call3_arg1.get_start_byte() == 54);
                CHECK(call3_arg1.get_end_byte() == 55);
                CHECK(call3_arg1.get_start_point() == ts::Point{.row = 4, .column = 10});
                CHECK(call3_arg1.get_end_point() == ts::Point{.row = 4, .column = 11});
                CHECK(call3_arg1.get_text(source_code) == "3"s);
            }

            {
                ts::Node call4 = else_branch.get_named_child(1);
                CHECK(call4.get_type() == "function_call"s);
                CHECK(call4.get_start_byte() == 61);
                CHECK(call4.get_end_byte() == 69);
                CHECK(call4.get_start_point() == ts::Point{.row = 5, .column = 4});
                CHECK(call4.get_end_point() == ts::Point{.row = 5, .column = 12});
                CHECK(call4.get_text(source_code) == "print(4)"s);

                ts::Node call4_ident = call4.get_named_child(0);
                CHECK(call4_ident.get_type() == "identifier"s);
                CHECK(call4_ident.get_start_byte() == 61);
                CHECK(call4_ident.get_end_byte() == 66);
                CHECK(call4_ident.get_start_point() == ts::Point{.row = 5, .column = 4});
                CHECK(call4_ident.get_end_point() == ts::Point{.row = 5, .column = 9});
                CHECK(call4_ident.get_text(source_code) == "print"s);

                ts::Node call4_args = call4.get_named_child(1);
                CHECK(call4_args.get_type() == "arguments"s);
                CHECK(call4_args.get_named_child_count() == 1);

                ts::Node call4_arg1 = call4_args.get_named_child(0);
                CHECK(call4_arg1.get_type() == "number"s);
                CHECK(call4_arg1.get_start_byte() == 67);
                CHECK(call4_arg1.get_end_byte() == 68);
                CHECK(call4_arg1.get_start_point() == ts::Point{.row = 5, .column = 10});
                CHECK(call4_arg1.get_end_point() == ts::Point{.row = 5, .column = 11});
                CHECK(call4_arg1.get_text(source_code) == "4"s);
            }
        }

        FAIL();
    }
}

TEST_CASE("Tree-Sitter", "[parse]") {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_lua());

    const char* source_code = "1 + 2";
    TSTree* tree = ts_parser_parse_string(parser, NULL, source_code, strlen(source_code));

    TSNode root_node = ts_tree_root_node(tree);
    TSNode expr_node = ts_node_named_child(root_node, 0);
    TSNode bin_op_node = ts_node_named_child(expr_node, 0);
    TSNode number_1_node = ts_node_named_child(bin_op_node, 0);
    TSNode number_2_node = ts_node_named_child(bin_op_node, 1);

    CHECK(ts_node_type(root_node) == std::string{"program"});
    CHECK(ts_node_type(expr_node) == std::string{"expression"});
    CHECK(ts_node_type(bin_op_node) == std::string{"binary_operation"});
    CHECK(ts_node_named_child_count(bin_op_node) == 2);
    CHECK(ts_node_type(number_1_node) == std::string{"number"});
    CHECK(ts_node_type(number_2_node) == std::string{"number"});

    char* s = ts_node_string(root_node);

    INFO(s);

    free(s);
    ts_tree_delete(tree);
    ts_parser_delete(parser);
}
