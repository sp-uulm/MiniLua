#include <iostream>
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "tree-sitter/tree-sitter.hpp"
#include <catch2/catch.hpp>

TEST_CASE("Tree-Sitter Node navigation") {
    std::string source = R"#(if true then
    print(1)
    print(2)
else
    while true do
        print(3)
    end
    print(4)
end)#";

    ts::Parser parser;
    ts::Tree tree = parser.parse_string(source);

    {
        ts::Node root = tree.root_node();
        ts::Node print3 = root.child(0).named_child(3).named_child(0).named_child(1);
        REQUIRE(print3.text() == "print(3)");

        ts::Node three =
            root.child(0).named_child(3).named_child(0).named_child(1).named_child(1).named_child(
                0);
        REQUIRE(three.text() == "3");
    }

    BENCHMARK("get root node") {
        ts::Node root = tree.root_node();
        return root;
    };

    ts::Node root = tree.root_node();

    BENCHMARK("copy root node") {
        ts::Node root_copy = root;
        return root_copy;
    };

    BENCHMARK("navigate to print(3)") {
        ts::Node print3 = root.child(0).named_child(3).named_child(0).named_child(1);
        return print3;
    };

    BENCHMARK("navigate to 3") {
        ts::Node three =
            root.child(0).named_child(3).named_child(0).named_child(1).named_child(1).named_child(
                0);
        return three;
    };

    ts::Node print3 = root.child(0).named_child(3).named_child(0).named_child(1);

    BENCHMARK("navigate to 3 after visiting print(3)") {
        ts::Node three = print3.named_child(1).named_child(0);
        return three;
    };
}

TEST_CASE("Tree-Sitter Cursor navigation") {
    std::string source = R"#(if true then
    print(1)
    print(2)
else
    while true do
        print(3)
    end
    print(4)
end)#";

    ts::Parser parser;
    ts::Tree tree = parser.parse_string(source);

    {
        ts::Cursor cursor = ts::Cursor(tree);
        REQUIRE(cursor.goto_first_child());
        REQUIRE(cursor.goto_first_named_child());
        REQUIRE(cursor.goto_next_named_sibling());
        REQUIRE(cursor.goto_next_named_sibling());
        REQUIRE(cursor.goto_next_named_sibling());
        REQUIRE(cursor.goto_first_named_child());
        REQUIRE(cursor.goto_first_named_child());
        REQUIRE(cursor.goto_next_named_sibling());
        ts::Node print3 = cursor.current_node();
        REQUIRE(print3.text() == "print(3)");

        REQUIRE(cursor.goto_first_named_child());
        REQUIRE(cursor.goto_next_named_sibling());
        REQUIRE(cursor.goto_first_named_child());
        ts::Node three = cursor.current_node();
        REQUIRE(three.text() == "3");
    }

    BENCHMARK("create ursor") { return ts::Cursor(tree); };

    {
        ts::Cursor cursor = ts::Cursor(tree);
        BENCHMARK("copy cursor") { return ts::Cursor(cursor); };
    }

    {
        BENCHMARK_ADVANCED("resetting cursor")(Catch::Benchmark::Chronometer meter) {
            ts::Cursor cursor = ts::Cursor(tree);
            cursor.goto_first_child();
            meter.measure([&tree, &cursor] { cursor.reset(tree); });
        };
    }

    {
        ts::Cursor cursor = ts::Cursor(tree);
        BENCHMARK("navigate to print(3)") {
            cursor.goto_first_child();
            cursor.goto_first_named_child();
            cursor.goto_next_named_sibling();
            cursor.goto_next_named_sibling();
            cursor.goto_next_named_sibling();
            cursor.goto_first_named_child();
            cursor.goto_first_named_child();
            cursor.goto_next_named_sibling();
            ts::Node print3 = cursor.current_node();
            return print3;
        };
    }

    {
        ts::Cursor cursor = ts::Cursor(tree);
        BENCHMARK("navigate to 3") {
            cursor.goto_first_named_child();
            cursor.goto_next_named_sibling();
            cursor.goto_next_named_sibling();
            cursor.goto_next_named_sibling();
            cursor.goto_first_named_child();
            cursor.goto_first_named_child();
            cursor.goto_next_named_sibling();

            cursor.goto_first_named_child();
            cursor.goto_next_named_sibling();
            cursor.goto_first_named_child();
            ts::Node three = cursor.current_node();
            return three;
        };
    }

    {
        ts::Cursor cursor = ts::Cursor(tree);
        cursor.goto_first_named_child();
        cursor.goto_next_named_sibling();
        cursor.goto_next_named_sibling();
        cursor.goto_next_named_sibling();
        cursor.goto_first_named_child();
        cursor.goto_first_named_child();
        cursor.goto_next_named_sibling();

        BENCHMARK("navigate to 3 after visiting print(3)") {
            cursor.goto_first_named_child();
            cursor.goto_next_named_sibling();
            cursor.goto_first_named_child();
            ts::Node one = cursor.current_node();
            return one;
        };
    }
}

TEST_CASE("Tree-Sitter Query navigation") {
    std::string source = R"#(if true then
    print(1)
    print(2)
else
    while true do
        print(3)
    end
    print(4)
end)#";

    ts::Parser parser;
    ts::Tree tree = parser.parse_string(source);

    INFO(tree.root_node());
    // FAIL();

    {
        ts::Node root = tree.root_node();
        ts::Query print3_query{"(function_call (identifier) @function (arguments (number) @number)) @call"};
        ts::QueryCursor cursor{tree};
        cursor.exec(print3_query);
        auto matches = cursor.matches();
        INFO(matches);
        // FAIL();
        ts::Node print3 = root.child(0).named_child(3).named_child(0).named_child(1);
        REQUIRE(print3.text() == "print(3)");

        ts::Node three =
            root.child(0).named_child(3).named_child(0).named_child(1).named_child(1).named_child(
                0);
        REQUIRE(three.text() == "3");
    }

    BENCHMARK("create query") {
        ts::Query function_query{"(function_call (identifier) @function (arguments (number) @number)) @call"};
        return function_query;
    };

    BENCHMARK("create query cursor") {
        return ts::QueryCursor{tree};
    };

    ts::Query function_query{"(function_call (identifier) @function (arguments (number) @number)) @call"};
    ts::QueryCursor cursor{tree};

    BENCHMARK("execute query") {
        cursor.exec(function_query);
    };

    BENCHMARK("exec and retrieve all matches") {
        cursor.exec(function_query);
        std::vector<ts::Match> matches = cursor.matches();
        return matches;
    };

    BENCHMARK("exec and retrieve all captures from all matches") {
        cursor.exec(function_query);
        std::vector<ts::Match> matches = cursor.matches();
        std::vector<ts::Capture> captures;

        for (const auto& match : matches) {
            const auto& new_captures = match.captures;
            std::copy(new_captures.cbegin(), new_captures.cend(), std::back_inserter(captures));
        }

        return captures;
    };

    BENCHMARK("exec and find print(3) call in matches") {
        cursor.exec(function_query);
        std::vector<ts::Match> matches = cursor.matches();

        for (const auto& match : matches) {
            const auto& captures = match.captures;
            if (captures[1].node.text() == "print" && captures[2].node.text() == "3") {
                return std::make_optional<ts::Node>(captures[0].node);
            }
        }

        throw std::runtime_error("should not reach here");
    };

    ts::Query query_if{"(program (if_statement (condition_expression) @cond (_)* @body (else) @else_body))"};
    ts::Query query_while{"(while_statement (condition_expression) @cond (_)* @body)"};
    ts::Query query_call{"(function_call (identifier) @function (arguments) @args)"};
    ts::Query query_number{"(number) @num"};

    BENCHMARK("navigate to 3") {
        cursor.exec(query_if);
        ts::Match match_if = cursor.next_match().value();
        const ts::Capture capture_else = match_if.capture_with_index(2).value();

        cursor.exec(query_while, capture_else.node);
        ts::Match match_while = cursor.next_match().value();
        const ts::Capture capture_while_body = match_while.capture_with_index(1).value();

        cursor.exec(query_call, capture_while_body.node);
        ts::Match match_call = cursor.next_match().value();
        const ts::Capture capture_call_args = match_call.capture_with_index(1).value();

        cursor.exec(query_number, capture_call_args.node);
        ts::Match match_number = cursor.next_match().value();
        const ts::Capture capture_number = match_number.capture_with_index(0).value();
        ts::Node number = capture_number.node;

        return number;
    };

    cursor.exec(query_if);
    ts::Match match_if = cursor.next_match().value();
    const ts::Capture capture_else = match_if.capture_with_index(2).value();

    cursor.exec(query_while, capture_else.node);
    ts::Match match_while = cursor.next_match().value();
    const ts::Capture capture_while_body = match_while.capture_with_index(1).value();
    ts::Node print3 = capture_while_body.node;
    REQUIRE(print3.text() == "print(3)");

    BENCHMARK("navigate to 3 after visiting print(3)") {
        cursor.exec(query_call, print3);
        ts::Match match_call = cursor.next_match().value();
        const ts::Capture capture_call_args = match_call.capture_with_index(1).value();

        cursor.exec(query_number, capture_call_args.node);
        ts::Match match_number = cursor.next_match().value();
        const ts::Capture capture_number = match_number.capture_with_index(0).value();
        ts::Node number = capture_number.node;

        return number;
    };
}
