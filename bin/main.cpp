#include "MiniLua/MiniLua.hpp"
#include "MiniLua/interpreter.hpp"

#include <fstream>
#include <iostream>

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

    if (auto result = interpreter.parse_file(argv[index]); !result) {
        std::cerr << "Failed to parse\nErrors:\n";
        for (const auto& error : result.errors) {
            std::cerr << " - " << error << "\n";
        }
        return 3;
    }

    try {
        auto result = interpreter.evaluate();
        if (!quiet) {
            std::cerr << "Terminated successfullly with value:\n\t" << result.value.to_literal()
                      << "\n";
            if (result.source_change.has_value()) {
                std::cerr << "and source changes:\n\t" << result.source_change.value() << "\n";
            }

            // TODO pretty print
            std::cerr << "\nThe value had origin: " << result.value.origin() << "\n";
        }
    } catch (const minilua::InterpreterException& e) {
        // std::cerr << "Evaluation failed:\n";
        e.print_stacktrace(std::cerr);
        return 4;
    }
}
