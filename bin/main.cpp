#include "MiniLua/MiniLua.hpp"
#include "MiniLua/interpreter.hpp"

#include <fstream>
#include <iostream>

std::string read_input_from_file(std::string path) {
    std::ifstream ifs;
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs.open(path);
    return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

auto main(int argc, char* argv[]) -> int {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program.lua>\n";
        return 1;
    }

    std::string source_code;
    try {
        source_code = read_input_from_file(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load file: " << e.what() << "\n";
        return 2;
    }

    minilua::Interpreter interpreter;
    interpreter.config().all(true);
    if (auto result = interpreter.parse(source_code); !result) {
        std::cerr << "Failed to parse\nErrors:\n";
        for (const auto& error : result.errors) {
            std::cerr << " - " << error << "\n";
        }
        return 3;
    }

    try {
        auto result = interpreter.evaluate();
        std::cerr << "Terminated successfullly with value:\n" << result.value << "\n";

        // TODO source changes
    } catch (const minilua::InterpreterException& e) {
        std::cerr << "Evaluation failed with: " << e.what() << "\n";
        return 4;
    }
}
