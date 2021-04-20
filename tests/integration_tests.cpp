#include <catch2/catch.hpp>
#include <cstdio>
#include <fstream>
#include <ftw.h>
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

TEST_CASE("interpreter does not return functions") {
    SECTION("plain function") {
        minilua::Interpreter interpreter;
        interpreter.parse("return print");
        auto result = interpreter.evaluate();
        REQUIRE(result.value.is_nil());
    }

    SECTION("function in table") {
        minilua::Interpreter interpreter;
        interpreter.parse(R"(return {print = print})");
        auto result = interpreter.evaluate();
        REQUIRE(result.value.is_nil());
    }
}

static std::vector<int> open_fds;
static std::vector<std::string> open_files;
static auto ftw_callback(const char* fpath, const struct stat* sb, int typeflag) -> int {
    if (typeflag == FTW_F) {
        int num;

        sscanf(fpath, "/proc/self/fd/%d", &num);
        open_fds.push_back(num);

        // it not possible to get the link size for special files in e.g. /proc
        // so we just use PATH_MAX
        // this might truncate the filename but PATH_MAX should be big enough
        int bufsize = PATH_MAX + 1;
        char buf[bufsize];
        int nbytes = readlink(fpath, buf, bufsize);
        if (nbytes == -1) {
            throw std::runtime_error("failed to read symlink: "s + fpath);
        }
        buf[nbytes] = '\0';
        std::string link_target(buf);
        open_files.push_back(link_target);
    }
    return 0;
}

static auto read_proc_self_fd() -> std::vector<int> {
    open_fds.clear();
    open_files.clear();
    ftw("/proc/self/fd", ftw_callback, 1);
    return open_fds;
}

TEST_CASE("io all files are closed when the interpreter quits") {
    read_proc_self_fd();
    auto pre_run_open_fds = open_fds;
    auto pre_run_open_files = open_files;

    minilua::Interpreter interpreter;
    interpreter.parse(R"(io.open("/tmp/test.txt"))");
    auto result = interpreter.evaluate();
    REQUIRE(result.value.is_nil());

    read_proc_self_fd();
    auto post_run_open_fds = open_fds;
    auto post_run_open_files = open_files;

    REQUIRE_THAT(post_run_open_fds, Catch::Matchers::UnorderedEquals(pre_run_open_fds));
    REQUIRE_THAT(post_run_open_files, Catch::Matchers::UnorderedEquals(pre_run_open_files));
}

TEST_CASE("old Environment leaks", "[interpreter][leaks]") {
    static_assert(std::is_move_constructible<lua::rt::Environment>());

    auto env = std::make_shared<lua::rt::Environment>(nullptr);

    env->populate_stdlib();
}
