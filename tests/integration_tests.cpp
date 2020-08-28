#include <catch2/catch.hpp>
#include <string>
#include <variant>

#include "MiniLua/luaparser.hpp"
#include "MiniLua/luainterpreter.hpp"

void add_force_function_to_env(const std::shared_ptr<lua::rt::Environment>& env) {
    env->assign(string {"force"}, make_shared<lua::rt::cfunction>([](const lua::rt::vallist& args) -> lua::rt::cfunction::result {
        if (args.size() != 2) {
            return lua::rt::vallist{lua::rt::nil(), string {"wrong number of arguments (expected 2)"}};
        }

        auto source_changes = args[0].forceValue(args[1]);

        if (source_changes.has_value()) {
            return {source_changes.value()};
        }

        return {};
    }), false);

}

std::string parse_eval_update(std::string program) {
    // parse
    LuaParser parser;
    PerformanceStatistics ps;
    const auto result = parser.parse(program, ps);

    // check for error
    if (std::holds_alternative<std::string>(result)) {
        INFO(std::get<std::string>(result));
        CHECK(false);
    }

    // evaluate
    const auto& ast = std::get<LuaChunk>(result);
    auto env = std::make_shared<lua::rt::Environment>(nullptr);
    env->populate_stdlib();
    add_force_function_to_env(env);

    lua::rt::ASTEvaluator eval;
    auto eval_result = ast->accept(eval, env);

    if (std::holds_alternative<std::string>(eval_result)) {
        INFO(std::get<std::string>(eval_result));
        CHECK(false);
    }

    // update ast
    if (auto sc = get_sc(eval_result)) {
        return get_string((*sc)->apply(parser.tokens));
    }

    // TODO (why) is this needed?
    env->clear();

    return program;
}

TEST_CASE("parse, eval, update", "[parse]") {
    SECTION("simple for") {
        const std::string program = "for i=1, 10, 1 do \n    print('hello world ', i)\nend";
        const auto result = parse_eval_update(program);
        REQUIRE(result == program);
    }

    SECTION("force value") {
        const std::string program = "force(2, 3)";
        const auto result = parse_eval_update(program);
        REQUIRE(result == "force(3, 3)");
    }
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
    ts_parser_delete(parser);
}
