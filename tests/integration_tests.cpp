#include <catch2/catch.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <variant>

#include "MiniLua/interpreter.hpp"
#include "MiniLua/luainterpreter.hpp"
#include "MiniLua/luaparser.hpp"

void add_force_function_to_env(const std::shared_ptr<lua::rt::Environment>& env) {
    env->assign(
        string{"force"},
        make_shared<lua::rt::cfunction>(
            [](const lua::rt::vallist& args) -> lua::rt::cfunction::result {
                if (args.size() != 2) {
                    return lua::rt::vallist{
                        lua::rt::nil(), string{"wrong number of arguments (expected 2)"}};
                }

                auto source_changes = args[0].forceValue(args[1]);

                if (source_changes.has_value()) {
                    return {source_changes.value()};
                }

                return {};
            }),
        false);
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

std::string read_input_from_file(std::string path) {
    std::ifstream ifs;
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs.open(path);
    return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

// TODO fix the memory leaks
TEST_CASE("parse, eval, update", "[parse][leaks]") {
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

    SECTION("COMMENTS") {
        const std::string program = "print('test')\n --print('normal comment')\nprint('hello')";
        const auto result = parse_eval_update(program);
        REQUIRE(result == program);
    }
}

TEST_CASE("unit_tests lua files") {
    std::vector<std::string> test_files{
        "literals/bools.lua",
        "literals/numbers.lua",
        "literals/string.lua",
        "literals/table.lua",
        "expressions/binary_operations.lua",
        "expressions/unary_operations.lua",
        "statements/if.lua",
        "statements/while.lua",
        "statements/repeat_until.lua",
        "statements/functions.lua",
        "local_variables.lua",
        "counter.lua",
        "metatables/general.lua",
        "metatables/add.lua",
        "metatables/tostring.lua",
        "metatables/comparison.lua",
        "metatables/call.lua",
    };
    // NOTE: exptects to be run from build directory
    for (const auto& file : test_files) {
        std::string path = "../luaprograms/unit_tests/" + file;
        DYNAMIC_SECTION("File: " << path) {
            // TODO remove once comments work
            std::string program = read_input_from_file(path);

            while (true) {
                auto start_pos = program.find("--");
                if (start_pos == std::string::npos) {
                    break;
                }

                auto end_pos = program.find('\n', start_pos);
                if (end_pos == std::string::npos) {
                    end_pos = program.size() - 1;
                }

                auto count = end_pos - start_pos + 1;
                program.replace(start_pos, count, "");
            }

            minilua::Interpreter interpreter;
            REQUIRE(interpreter.parse(program));
            auto result = interpreter.evaluate();
            REQUIRE(!result.source_change.has_value());
        }
    }
}

TEST_CASE("whole lua-programs", "[.hide]") {
    SECTION("programs with function calls") {
        std::vector<std::string> test_files{
            "BepposBalloons.lua",
            "FragmeentedFurniture.lua",
            "FragmentedFurniture_withoutMethods.lua",
            "HelplessHuman.lua",
            "LottaLaps.lua",
            "luaToStringFunctionExample.lua",
            "simple.lua"};
        for (const auto& file : test_files) {
            std::string path = "../luaprograms/" + file;
            DYNAMIC_SECTION("File: " << path) {
                const std::string program = read_input_from_file(path);

                // old parser
                // const auto result = parse_eval_update(program);
                // REQUIRE(result == program);

                minilua::Interpreter interpreter;
                // interpreter.config().all(true);
                interpreter.parse(program);
                auto result = interpreter.evaluate();
                REQUIRE(!result.source_change.has_value());
            }
        }
    }

    SECTION("Lua-program without functions") {
        const std::string program =
            read_input_from_file("../luaprograms/FragmentedFurniture_withoutMethods.lua");
        const auto result = parse_eval_update(program);
        REQUIRE(result == program);
    }
}
TEST_CASE("old Environment leaks", "[interpreter][leaks]") {
    static_assert(std::is_move_constructible<lua::rt::Environment>());

    auto env = std::make_shared<lua::rt::Environment>(nullptr);

    env->populate_stdlib();
}
