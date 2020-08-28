#include <catch2/catch.hpp>
#include <cstring>
#include <tree_sitter/api.h>

#include "tree-sitter/tree-sitter.hpp"

TEST_CASE("Tree-Sitter-Wrapper", "[parse]") {
    ts::Parser parser;

    std::string source_code = "1 + 2";
    ts::Tree tree = parser.parse_string(source_code);

    TSNode root_node = tree.get_root_node();
    TSNode expr_node = ts_node_named_child(root_node, 0);
    TSNode bin_op_node = ts_node_named_child(expr_node, 0);
    TSNode number_1_node = ts_node_named_child(bin_op_node, 0);
    TSNode number_2_node = ts_node_named_child(bin_op_node, 1);

    REQUIRE(ts_node_type(root_node) == std::string{"program"});
    REQUIRE(ts_node_type(expr_node) == std::string{"expression"});
    REQUIRE(ts_node_type(bin_op_node) == std::string{"binary_operation"});
    REQUIRE(ts_node_named_child_count(bin_op_node) == 2);
    REQUIRE(ts_node_type(number_1_node) == std::string{"number"});
    REQUIRE(ts_node_type(number_2_node) == std::string{"number"});

    char* s = ts_node_string(root_node);

    INFO(s);

    free(s);
}

TEST_CASE("Tree-Sitter", "[parse]") {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_lua());

    const char* source_code = "1 + 2";
    TSTree* tree = ts_parser_parse_string(
            parser,
            NULL,
            source_code,
            strlen(source_code));

    TSNode root_node = ts_tree_root_node(tree);
    TSNode expr_node = ts_node_named_child(root_node, 0);
    TSNode bin_op_node = ts_node_named_child(expr_node, 0);
    TSNode number_1_node = ts_node_named_child(bin_op_node, 0);
    TSNode number_2_node = ts_node_named_child(bin_op_node, 1);

    REQUIRE(ts_node_type(root_node) == std::string{"program"});
    REQUIRE(ts_node_type(expr_node) == std::string{"expression"});
    REQUIRE(ts_node_type(bin_op_node) == std::string{"binary_operation"});
    REQUIRE(ts_node_named_child_count(bin_op_node) == 2);
    REQUIRE(ts_node_type(number_1_node) == std::string{"number"});
    REQUIRE(ts_node_type(number_2_node) == std::string{"number"});

    char* s = ts_node_string(root_node);

    INFO(s);

    free(s);
    ts_tree_delete(tree);
}

