#include <catch2/catch.hpp>
#include <string>
#include <variant>
#include <filesystem>
#include <iostream>
#include <fstream>

#include "MiniLua/luainterpreter.hpp"
#include "MiniLua/luaparser.hpp"

namespace fs = std::filesystem;

void add_force_function_to_env(const std::shared_ptr<lua::rt::Environment>& env) {
    env->assign(string{"force"},
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

std::string read_input_from_file(std::string path){
    std::ifstream ifs;
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs.open(path);
    return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
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

    SECTION("COMMENTS"){
        const std::string program = "print('test')\n --print('normal comment')\nprint('hello')";
        const auto result = parse_eval_update(program);
        REQUIRE(result == program);
    }
}

TEST_CASE("whole lua-programs") {
    SECTION("programs with function calls"){
        /*
        const std::vector<std::string> files;

        //Find all Files in directory
        for (const auto& entry : fs::directory_iterator("../luaprograms")){
            //cout << typeid(entry.path()).name() << "\n";
            //files.push_back(entry.path());
            DYNAMIC_SECTION("File: " << entry.path()){
                const std::string program = read_input_from_file(entry.path());
                const auto result = parse_eval_update(program);
                REQUIRE(result == program);
            }
        }
        */
    }

    SECTION("Lua-program without functions"){
        const std::string program = read_input_from_file("../luaprograms/FragmentedFurniture_withoutMethods.lua");
        const auto result = parse_eval_update(program);
        REQUIRE(result == program);
    }
}
