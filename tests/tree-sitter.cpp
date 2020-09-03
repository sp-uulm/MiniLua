#include <catch2/catch.hpp>
#include <cstring>
#include <tree_sitter/api.h>
#include <type_traits>

#include "tree-sitter/tree-sitter.hpp"

using namespace std::string_literals;

TEST_CASE("Navigation", "[tree-sitter][!hide]") {
    // This is a possible design of how to use tree-sitter in the interpreter.
    // But of course this would be split up over multiple functions
    // and could use better variable names because of that.
    // Each of the nested ifs would probably be a separate function, so the
    // nesting would not be very deep.

    class Expression {
        // ...
    };
    enum class BinOp {
        Add,
        // ...
    };
    class BinaryOperation {
        ts::Node left_node;
        ts::Node right_node;
        ts::Node op_node;

    public:
        BinaryOperation(ts::Node node)
            : left_node(node.child(0)), right_node(node.child(2)), op_node(node.child(1)) {
            if (node.type() != "binary_operation"s) {
                throw std::runtime_error("not a binary_operation node");
            }
        }

        Expression left() { return Expression(/* this->left_node */); }
        Expression right() { return Expression(/* this->right_node */); }
        BinOp op() {
            // switch (this->op_node->get_text())
            // ...
            return BinOp::Add;
        }
    };

    std::string source = "1 + 2";
    ts::Parser parser;
    ts::Tree tree = parser.parse_string(source);
    ts::Node root_node = tree.root_node();
    assert(root_node.type() == "program"s);

    ts::Node child = root_node.named_child(0);
    // check all "root" types
    if (child.type() == "expression"s) {
        ts::Node next_child = child.named_child(0);
        // check all expression types
        if (next_child.type() == "binary_operation"s) {
            BinaryOperation bin_op = BinaryOperation(next_child);
            Expression left_expr = bin_op.left();
            // ... evaluate left_expr

            // check right expression
            Expression right_expr = bin_op.right();
            // ... evaluate right_expr

            // check operator
            BinOp op = bin_op.op();
            if (op == BinOp::Add) {
                // ... evaluate addition
            } else /* if (...) */ {
                // exception: unknown operator
            }
        } else /* if (...) */ {
            // exception: unknown expression
        }
    } else /* if (...) */ {
        // exception: unknown root node
    }
}

TEST_CASE("Print", "[tree-sitter][!hide]") {
    ts::Parser parser;

    std::string source = "print(1+2)";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();

    INFO(root.as_s_expr());
    FAIL();
}

TEST_CASE("language is compatible with tree-sitter", "[tree-sitter]") {
    ts::Language lang = ts::LUA_LANGUAGE;
    REQUIRE(TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION <= lang.version());
    REQUIRE(TREE_SITTER_LANGUAGE_VERSION >= lang.version());
}

TEST_CASE("language can list all fields", "[tree-sitter][!hide]") {
    ts::Language lang = ts::LUA_LANGUAGE;

    CAPTURE(lang.field_count());

    for (auto field_id = 1; field_id < lang.field_count() + 1; ++field_id) {
        const char* name = lang.field_name(field_id);
        UNSCOPED_INFO(field_id << ": " << name);
    }

    // FAIL();
}

TEST_CASE("language can list all node types", "[tree-sitter][!hide]") {
    ts::Language lang = ts::LUA_LANGUAGE;

    CAPTURE(lang.node_type_count());

    for (auto type_id = 0; type_id < lang.node_type_count(); ++type_id) {
        bool is_named = lang.node_type_kind(type_id) == ts::TypeKind::Named;
        const char* name = lang.node_type_name(type_id);
        ts::TypeId id = lang.node_type_id(name, is_named);
        if (is_named && type_id == id) {
            UNSCOPED_INFO(type_id << ": " << name);
        }
    }

    // FAIL();
}

TEST_CASE("tree can be copied", "[tree-sitter]") {
    ts::Parser parser;
    std::string source = "1 + 2";
    const ts::Tree tree = parser.parse_string(source);

    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    ts::Tree tree_copy = tree;
    CHECK(tree.source() == tree_copy.source());
    // root node is not equal because the tree is different
    CHECK(&tree.root_node().tree() != &tree_copy.root_node().tree());
}

TEST_CASE("trees can be edited", "[tree-sitter]") {
    ts::Parser parser;

    SECTION("changing an integer literal") {
        std::string source = "1 + 2";
        ts::Tree tree = parser.parse_string(source);

        INFO("Pre edit: " << tree.root_node().as_s_expr());

        // check pre-condition on tree
        ts::Node one_node = tree.root_node().named_child(0).named_child(0).child(0);
        CHECK(one_node.type() == "number"s);
        CHECK(one_node.text() == "1"s);

        // create an edit
        ts::Range one_range = one_node.range();
        INFO("Range of old 'one_node' " << one_node.range());
        ts::Edit edit{
            .range = one_range,
            .replacement = "15"s,
        };

        // apply the edit
        tree.edit({edit});
        // NOTE: don't use one_node after this

        INFO("Post edit: " << tree.root_node().as_s_expr());

        CHECK(tree.source() == "15 + 2"s);

        ts::Node new_one_node = tree.root_node().named_child(0).named_child(0).child(0);
        CHECK(new_one_node.type() == "number"s);
        INFO("Range of new 'one_node' " << new_one_node.range());
        CHECK(new_one_node.text() == "15"s);
    }

    SECTION("changing multiple integer literals") {
        std::string source = "1 + 2";
        ts::Tree tree = parser.parse_string(source);

        INFO("Pre edit: " << tree.root_node().as_s_expr());

        // check pre-condition on tree
        ts::Node one_node = tree.root_node().named_child(0).named_child(0).child(0);
        CHECK(one_node.type() == "number"s);
        CHECK(one_node.text() == "1"s);
        ts::Node two_node = tree.root_node().named_child(0).named_child(0).child(2);
        CHECK(two_node.type() == "number"s);
        CHECK(two_node.text() == "2"s);

        // create an edit
        ts::Range one_range = one_node.range();
        INFO("Range of old 'one_node' " << one_node.range());
        ts::Edit edit_one{
            .range = one_range,
            .replacement = "15"s,
        };
        ts::Range two_range = two_node.range();
        INFO("Range of old 'two_node' " << two_node.range());
        ts::Edit edit_two{
            .range = two_range,
            .replacement = "7"s,
        };

        // apply the edit
        tree.edit({edit_one, edit_two});
        // NOTE: don't use one_node or two_node after this

        INFO("Post edit: " << tree.root_node().as_s_expr());

        CHECK(tree.source() == "15 + 7"s);

        ts::Node new_one_node = tree.root_node().named_child(0).named_child(0).child(0);
        CHECK(new_one_node.type() == "number"s);
        INFO("Range of new 'one_node' " << new_one_node.range());
        CHECK(new_one_node.text() == "15"s);
        ts::Node new_two_node = tree.root_node().named_child(0).named_child(0).child(2);
        CHECK(new_two_node.type() == "number"s);
        INFO("Range of new 'two_node' " << new_two_node.range());
        CHECK(new_two_node.text() == "7"s);
    }

    SECTION("changing multiple integer literals over multiple lines") {
        std::string source = R"#(local a = 1
local b = 2
return a + b)#";
        ts::Tree tree = parser.parse_string(source);

        INFO("Pre edit: " << tree.root_node().as_s_expr());

        // check pre-condition on tree
        ts::Node one_node = tree.root_node().named_child(0).named_child(1);
        CHECK(one_node.type() == "number"s);
        CHECK(one_node.text() == "1"s);
        ts::Node two_node = tree.root_node().named_child(1).named_child(1);
        CHECK(two_node.type() == "number"s);
        CHECK(two_node.text() == "2"s);

        // create an edit
        ts::Range one_range = one_node.range();
        INFO("Range of old 'one_node' " << one_node.range());
        ts::Edit edit_one{
            .range = one_range,
            .replacement = "15"s,
        };
        ts::Range two_range = two_node.range();
        INFO("Range of old 'two_node' " << two_node.range());
        ts::Edit edit_two{
            .range = two_range,
            .replacement = "7"s,
        };

        // apply the edit
        std::vector<ts::Range> changed_ranges = tree.edit({edit_two, edit_one});
        // NOTE: don't use one_node or two_node after this

        INFO("Post edit: " << tree.root_node().as_s_expr());

        std::string expected = R"#(local a = 15
local b = 7
return a + b)#";
        CHECK(tree.source() == expected);

        ts::Node new_one_node = tree.root_node().named_child(0).named_child(1);
        CHECK(new_one_node.type() == "number"s);
        INFO("Range of new 'one_node' " << new_one_node.range());
        CHECK(new_one_node.text() == "15"s);
        ts::Node new_two_node = tree.root_node().named_child(1).named_child(1);
        CHECK(new_two_node.type() == "number"s);
        INFO("Range of new 'two_node' " << new_two_node.range());
        CHECK(new_two_node.text() == "7"s);
    }
}

TEST_CASE("Tree-Sitter detects errors", "[tree-sitter][parse]") {
    ts::Parser parser;

    SECTION("correct code does not have an error") {
        std::string source = "1 + 2";
        ts::Tree tree = parser.parse_string(source);
        ts::Node root = tree.root_node();

        INFO(root.as_s_expr());
        CHECK(!root.has_error());
    }

    SECTION("missing operands are detected") {
        std::string source = "1 +";
        ts::Tree tree = parser.parse_string(source);
        ts::Node root = tree.root_node();

        INFO(root.as_s_expr());
        CHECK(root.has_error());
    }
}

TEST_CASE("Cursor", "[tree-sitter]") {
    static_assert(std::is_nothrow_copy_constructible_v<ts::Cursor>);
    static_assert(std::is_nothrow_copy_assignable_v<ts::Cursor>);
    static_assert(!std::is_nothrow_move_constructible_v<ts::Cursor>);
    static_assert(!std::is_nothrow_move_assignable_v<ts::Cursor>);

    ts::Parser parser;

    std::string source = "1 + 2";
    ts::Tree tree = parser.parse_string(source);

    SECTION("can walk a tree") {
        ts::Cursor cursor{tree};

        REQUIRE(cursor.current_node().type() == "program"s);
        REQUIRE(cursor.goto_first_named_child());
        CHECK(cursor.current_node().type() == "expression"s);
        REQUIRE(cursor.goto_first_named_child());
        CHECK(cursor.current_node().type() == "binary_operation"s);
        REQUIRE(cursor.goto_first_named_child());
        CHECK(cursor.current_node().type() == "number"s);
        CHECK(cursor.current_node().text() == "1"s);
        REQUIRE(cursor.goto_next_named_sibling());
        CHECK(cursor.current_node().type() == "number"s);
        CHECK(cursor.current_node().text() == "2"s);
    }

    SECTION("can be copied") {
        ts::Cursor cursor{tree};
        ts::Cursor cursor_copy{cursor};   // NOLINT
        ts::Cursor cursor_copy2 = cursor; // NOLINT
    }
}

TEST_CASE("Node", "[tree-sitter]") {
    static_assert(std::is_nothrow_copy_constructible_v<ts::Node>);
    static_assert(std::is_nothrow_move_constructible_v<ts::Node>);
    // this makes moves redundant
    static_assert(std::is_trivially_copyable_v<ts::Node>);

    ts::Parser parser;

    std::string source = "1 + 2";
    ts::Tree tree = parser.parse_string(source);
    ts::Node root = tree.root_node();

    SECTION("can be copied") {
        ts::Node node_copy{root};   // NOLINT
        ts::Node node_copy2 = root; // NOLINT
    }

    SECTION("can retrieve origin tree") { REQUIRE(&tree == &root.tree()); }

    SECTION("type() returns a non-empty string") {
        REQUIRE(root.type() != nullptr);
        REQUIRE(std::strlen(root.type()) > 0);

        ts::Node expr = root.named_child(0);
        REQUIRE(expr.type() != nullptr);
        REQUIRE(std::strlen(expr.type()) > 0);

        ts::Node bin_op = expr.named_child(0);
        REQUIRE(bin_op.type() != nullptr);
        REQUIRE(std::strlen(bin_op.type()) > 0);

        ts::Node number_1 = bin_op.child(0);
        REQUIRE(number_1.type() != nullptr);
        REQUIRE(std::strlen(number_1.type()) > 0);

        ts::Node op = bin_op.child(1);
        REQUIRE(op.type() != nullptr);
        REQUIRE(std::strlen(op.type()) > 0);

        ts::Node number_2 = bin_op.child(2);
        REQUIRE(number_2.type() != nullptr);
        REQUIRE(std::strlen(number_2.type()) > 0);
    }

    SECTION("type_id() returns a non-zero type id") {
        REQUIRE(root.type_id() != 0);

        ts::Node expr = root.named_child(0);
        REQUIRE(expr.type_id() != 0);

        ts::Node bin_op = expr.named_child(0);
        REQUIRE(bin_op.type() != nullptr);

        ts::Node number_1 = bin_op.child(0);
        REQUIRE(number_1.type() != nullptr);

        ts::Node op = bin_op.child(1);
        REQUIRE(op.type() != nullptr);

        ts::Node number_2 = bin_op.child(2);
        REQUIRE(number_2.type() != nullptr);
    }

    SECTION("child methods return a null node only if there are no more children") {
        REQUIRE(!root.is_null());

        ts::Node expr = root.named_child(0);
        REQUIRE(!expr.is_null());

        ts::Node bin_op = expr.named_child(0);
        REQUIRE(!bin_op.is_null());

        ts::Node number_1 = bin_op.child(0);
        REQUIRE(!number_1.is_null());

        ts::Node op = bin_op.child(1);
        REQUIRE(!op.is_null());

        ts::Node number_2 = bin_op.child(2);
        REQUIRE(!number_2.is_null());

        REQUIRE(root.child(1).is_null());
        REQUIRE(root.child(5).is_null());
        REQUIRE(number_2.child(0).is_null());
    }

    SECTION("named_child only return named nodes") {
        ts::Node expr = root.named_child(0);
        REQUIRE(expr.is_named());

        ts::Node bin_op = expr.named_child(0);
        REQUIRE(bin_op.is_named());

        ts::Node number_1 = bin_op.named_child(0);
        REQUIRE(number_1.is_named());

        ts::Node op = bin_op.child(1);
        REQUIRE(!op.is_named());

        ts::Node number_2 = bin_op.named_child(1);
        REQUIRE(number_2.is_named());
    }
}

TEST_CASE("Tree-Sitter-Wrapper", "[tree-sitter][parser]") {
    SECTION("Simple addition") {
        ts::Parser parser;

        std::string source_code = "1 + 2";
        ts::Tree tree = parser.parse_string(source_code);

        ts::Node root_node = tree.root_node();
        INFO(root_node.as_s_expr());
        CHECK(root_node.type() == "program"s);

        ts::Node expr_node = root_node.child(0);
        CHECK(expr_node.type() == "expression"s);

        ts::Node bin_op_node = expr_node.named_child(0);
        CHECK(bin_op_node.type() == "binary_operation"s);
        CHECK(bin_op_node.named_child_count() == 2);
        CHECK(bin_op_node.start_byte() == 0);
        CHECK(bin_op_node.end_byte() == 5);
        CHECK(bin_op_node.start_point() == ts::Point{.row = 0, .column = 0});
        CHECK(bin_op_node.end_point() == ts::Point{.row = 0, .column = 5});

        ts::Node number_1_node = bin_op_node.named_child(0);
        CHECK(number_1_node.type() == "number"s);
        CHECK(number_1_node.start_byte() == 0);
        CHECK(number_1_node.end_byte() == 1);
        CHECK(number_1_node.start_point() == ts::Point{.row = 0, .column = 0});
        CHECK(number_1_node.end_point() == ts::Point{.row = 0, .column = 1});

        ts::Node number_2_node = bin_op_node.named_child(1);
        CHECK(number_2_node.type() == "number"s);
        CHECK(number_2_node.start_byte() == 4);
        CHECK(number_2_node.end_byte() == 5);
        CHECK(number_2_node.start_point() == ts::Point{.row = 0, .column = 4});
        CHECK(number_2_node.end_point() == ts::Point{.row = 0, .column = 5});
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

        ts::Node root_node = tree.root_node();
        INFO(root_node.as_s_expr());
        CHECK(root_node.type() == "program"s);

        ts::Node if_stmt = root_node.child(0);
        CHECK(if_stmt.type() == "if_statement"s);
        CHECK(if_stmt.named_child_count() == 4);

        ts::Node condition = if_stmt.named_child(0);
        CHECK(condition.type() == "condition_expression"s);
        CHECK(condition.named_child_count() == 1);

        ts::Node true_lit = condition.named_child(0);
        CHECK(true_lit.type() == "true"s);

        {
            ts::Node call1 = if_stmt.named_child(1);
            CHECK(call1.type() == "function_call"s);
            CHECK(call1.start_byte() == 17);
            CHECK(call1.end_byte() == 25);
            CHECK(call1.start_point() == ts::Point{.row = 1, .column = 4});
            CHECK(call1.end_point() == ts::Point{.row = 1, .column = 12});
            CHECK(call1.text() == "print(1)"s);

            ts::Node call1_ident = call1.named_child(0);
            CHECK(call1_ident.type() == "identifier"s);
            CHECK(call1_ident.start_byte() == 17);
            CHECK(call1_ident.end_byte() == 22);
            CHECK(call1_ident.start_point() == ts::Point{.row = 1, .column = 4});
            CHECK(call1_ident.end_point() == ts::Point{.row = 1, .column = 9});
            CHECK(call1_ident.text() == "print"s);

            ts::Node call1_args = call1.named_child(1);
            CHECK(call1_args.type() == "arguments"s);
            CHECK(call1_args.named_child_count() == 1);

            ts::Node call1_arg1 = call1_args.named_child(0);
            CHECK(call1_arg1.type() == "number"s);
            CHECK(call1_arg1.start_byte() == 23);
            CHECK(call1_arg1.end_byte() == 24);
            CHECK(call1_arg1.start_point() == ts::Point{.row = 1, .column = 10});
            CHECK(call1_arg1.end_point() == ts::Point{.row = 1, .column = 11});
            CHECK(call1_arg1.text() == "1"s);
        }

        {
            ts::Node call2 = if_stmt.named_child(2);
            CHECK(call2.type() == "function_call"s);
            CHECK(call2.start_byte() == 30);
            CHECK(call2.end_byte() == 38);
            CHECK(call2.start_point() == ts::Point{.row = 2, .column = 4});
            CHECK(call2.end_point() == ts::Point{.row = 2, .column = 12});
            CHECK(call2.text() == "print(2)"s);

            ts::Node call2_ident = call2.named_child(0);
            CHECK(call2_ident.type() == "identifier"s);
            CHECK(call2_ident.start_byte() == 30);
            CHECK(call2_ident.end_byte() == 35);
            CHECK(call2_ident.start_point() == ts::Point{.row = 2, .column = 4});
            CHECK(call2_ident.end_point() == ts::Point{.row = 2, .column = 9});
            CHECK(call2_ident.text() == "print"s);

            ts::Node call2_args = call2.named_child(1);
            CHECK(call2_args.type() == "arguments"s);
            CHECK(call2_args.named_child_count() == 1);

            ts::Node call2_arg1 = call2_args.named_child(0);
            CHECK(call2_arg1.type() == "number"s);
            CHECK(call2_arg1.start_byte() == 36);
            CHECK(call2_arg1.end_byte() == 37);
            CHECK(call2_arg1.start_point() == ts::Point{.row = 2, .column = 10});
            CHECK(call2_arg1.end_point() == ts::Point{.row = 2, .column = 11});
            CHECK(call2_arg1.text() == "2"s);
        }

        {
            ts::Node else_branch = if_stmt.named_child(3);
            CHECK(else_branch.type() == "else"s);
            CHECK(else_branch.start_byte() == 39);
            CHECK(else_branch.end_byte() == 69);
            CHECK(else_branch.start_point() == ts::Point{.row = 3, .column = 0});
            CHECK(else_branch.end_point() == ts::Point{.row = 5, .column = 12});
            CHECK(else_branch.named_child_count() == 2);

            {
                ts::Node call3 = else_branch.named_child(0);
                CHECK(call3.type() == "function_call"s);
                CHECK(call3.start_byte() == 48);
                CHECK(call3.end_byte() == 56);
                CHECK(call3.start_point() == ts::Point{.row = 4, .column = 4});
                CHECK(call3.end_point() == ts::Point{.row = 4, .column = 12});
                CHECK(call3.text() == "print(3)"s);

                ts::Node call3_ident = call3.named_child(0);
                CHECK(call3_ident.type() == "identifier"s);
                CHECK(call3_ident.start_byte() == 48);
                CHECK(call3_ident.end_byte() == 53);
                CHECK(call3_ident.start_point() == ts::Point{.row = 4, .column = 4});
                CHECK(call3_ident.end_point() == ts::Point{.row = 4, .column = 9});
                CHECK(call3_ident.text() == "print"s);

                ts::Node call3_args = call3.named_child(1);
                CHECK(call3_args.type() == "arguments"s);
                CHECK(call3_args.named_child_count() == 1);

                ts::Node call3_arg1 = call3_args.named_child(0);
                CHECK(call3_arg1.type() == "number"s);
                CHECK(call3_arg1.start_byte() == 54);
                CHECK(call3_arg1.end_byte() == 55);
                CHECK(call3_arg1.start_point() == ts::Point{.row = 4, .column = 10});
                CHECK(call3_arg1.end_point() == ts::Point{.row = 4, .column = 11});
                CHECK(call3_arg1.text() == "3"s);
            }

            {
                ts::Node call4 = else_branch.named_child(1);
                CHECK(call4.type() == "function_call"s);
                CHECK(call4.start_byte() == 61);
                CHECK(call4.end_byte() == 69);
                CHECK(call4.start_point() == ts::Point{.row = 5, .column = 4});
                CHECK(call4.end_point() == ts::Point{.row = 5, .column = 12});
                CHECK(call4.text() == "print(4)"s);

                ts::Node call4_ident = call4.named_child(0);
                CHECK(call4_ident.type() == "identifier"s);
                CHECK(call4_ident.start_byte() == 61);
                CHECK(call4_ident.end_byte() == 66);
                CHECK(call4_ident.start_point() == ts::Point{.row = 5, .column = 4});
                CHECK(call4_ident.end_point() == ts::Point{.row = 5, .column = 9});
                CHECK(call4_ident.text() == "print"s);

                ts::Node call4_args = call4.named_child(1);
                CHECK(call4_args.type() == "arguments"s);
                CHECK(call4_args.named_child_count() == 1);

                ts::Node call4_arg1 = call4_args.named_child(0);
                CHECK(call4_arg1.type() == "number"s);
                CHECK(call4_arg1.start_byte() == 67);
                CHECK(call4_arg1.end_byte() == 68);
                CHECK(call4_arg1.start_point() == ts::Point{.row = 5, .column = 10});
                CHECK(call4_arg1.end_point() == ts::Point{.row = 5, .column = 11});
                CHECK(call4_arg1.text() == "4"s);
            }
        }
    }
}

TEST_CASE("Tree-Sitter", "[tree-sitter][parser]") {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_lua());

    const char* source_code = "1 + 2";
    TSTree* tree = ts_parser_parse_string(parser, nullptr, source_code, strlen(source_code));

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
