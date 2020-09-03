#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "tree-sitter/tree-sitter.hpp"
#include <catch2/catch.hpp>

TEST_CASE("Tree-Sitter Node navigation") {
    std::string source = R"#(if true then
    print(1)
    print(2)
else
    print(3)
    print(4)
end)#";

    ts::Parser parser;
    ts::Tree tree = parser.parse_string(source);

    {
        ts::Node root = tree.root_node();
        ts::Node print1 = root.child(0).named_child(1);
        REQUIRE(print1.text() == "print(1)");

        ts::Node one = root.child(0).named_child(1).named_child(1).named_child(0);
        REQUIRE(one.text() == "1");
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

    BENCHMARK("navigate to print(1)") {
        ts::Node print1 = root.child(0).named_child(1);
        return print1;
    };

    BENCHMARK("navigate to 1") {
        ts::Node one = root.child(0).named_child(1).named_child(1).named_child(0);
        return one;
    };

    ts::Node print1 = root.child(0).named_child(1);

    BENCHMARK("navigate to 1 after visiting print(1)") {
        ts::Node one = print1.named_child(1).named_child(0);
        return one;
    };
}

TEST_CASE("Tree-Sitter Cursor navigation") {
    std::string source = R"#(if true then
    print(1)
    print(2)
else
    print(3)
    print(4)
end)#";

    ts::Parser parser;
    ts::Tree tree = parser.parse_string(source);

    {
        ts::Cursor cursor = ts::Cursor(tree);
        REQUIRE(cursor.goto_first_child());
        REQUIRE(cursor.goto_first_named_child());
        REQUIRE(cursor.goto_next_named_sibling());
        ts::Node print1 = cursor.current_node();
        REQUIRE(print1.text() == "print(1)");

        REQUIRE(cursor.goto_first_named_child());
        REQUIRE(cursor.goto_next_named_sibling());
        REQUIRE(cursor.goto_first_named_child());
        ts::Node one = cursor.current_node();
        REQUIRE(one.text() == "1");
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
        BENCHMARK("navigate to print(1)") {
            cursor.goto_first_child();
            cursor.goto_first_named_child();
            cursor.goto_next_named_sibling();
            ts::Node print1 = cursor.current_node();
            return print1;
        };
    }

    {
        ts::Cursor cursor = ts::Cursor(tree);
        BENCHMARK("navigate to 1") {
            cursor.goto_first_child();
            cursor.goto_first_named_child();
            cursor.goto_next_named_sibling();
            cursor.goto_first_named_child();
            cursor.goto_next_named_sibling();
            cursor.goto_first_named_child();
            ts::Node one = cursor.current_node();
            return one;
        };
    }

    {
        ts::Cursor cursor = ts::Cursor(tree);
        cursor.goto_first_child();
        cursor.goto_first_named_child();
        cursor.goto_next_named_sibling();

        BENCHMARK("navigate to 1 after visiting print(1)") {
            cursor.goto_first_named_child();
            cursor.goto_next_named_sibling();
            cursor.goto_first_named_child();
            ts::Node one = cursor.current_node();
            return one;
        };
    }
}
