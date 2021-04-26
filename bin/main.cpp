#include "MiniLua/MiniLua.hpp"
#include "MiniLua/interpreter.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

using namespace std::string_literals;

auto main(int argc, char* argv[]) -> int {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [--quiet][--trace] <program.lua>\n";
        return 1;
    }

    bool trace = false;
    bool quiet = false;
    size_t index = 1;

    if (argv[index] == "--quiet"s) {
        ++index;
        quiet = true;
    }
    if (argv[index] == "--trace"s) {
        ++index;
        trace = true;
    }

    minilua::Interpreter interpreter;
    interpreter.config().all(trace);

    auto parse_result = interpreter.parse_file(argv[index]);
    if (!parse_result) {
        std::cerr << "Failed to parse\nErrors:\n";
        for (const auto& error : parse_result.errors) {
            std::cerr << " - " << error << "\n";
        }
        return 3;
    }

    std::cerr << "Parsing took " << parse_result.elapsed_time << "ns\n";

    try {
        auto t_start = std::chrono::steady_clock::now();
        auto result = interpreter.evaluate();

        auto t_end = std::chrono::steady_clock::now();

        if (!quiet) {

            std::cerr << "Terminated successfullly with value:\n\t" << result.value.to_literal()
                      << "\n";
            if (result.source_change.has_value()) {
                std::cerr << "and source changes:\n\t" << result.source_change.value() << "\n";
            }

            // TODO pretty print
            std::cerr << "\nThe value had origin: " << result.value.origin() << "\n";

            auto time =
                std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count();
            std::cerr << "Interpreting took " << time << "ns\n";
        }
    } catch (const minilua::InterpreterException& e) {
        // std::cerr << "Evaluation failed:\n";
        e.print_stacktrace(std::cerr);
        return 4;
    }
}
