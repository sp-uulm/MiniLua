#include <cstdio>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "MiniLua/io.hpp"
#include "MiniLua/values.hpp"

namespace minilua::io {

auto static open_file(const String& filename, const String& mode) -> Vallist {
    std::FILE* file = std::fopen(filename.value.c_str(), mode.value.c_str());
    if (file == nullptr) {
        // TODO: difernciate between different errors
        return Vallist({Nil(), "could not open file", 2});
    }
    return Vallist();
}

auto open(const CallContext& ctx) -> Vallist {
    auto file = ctx.arguments().get(0);
    auto mode = ctx.arguments().get(1);

    return std::visit(
        overloaded{
            [](const String& file, Nil /*unused*/) -> Vallist {
                return open_file(file, std::get<String>("r"));
            },
            [](String file, const String& mode) -> Vallist {
                std::regex modes(R"(([rwa]\+?b?))");

                if (std::regex_match(mode.value, modes)) {
                    return open_file(std::move(file), mode);
                } else {
                    throw std::runtime_error("invalid mode");
                }
            },
            [](auto /*unused*/, auto /*unused*/) -> Vallist { throw std::runtime_error(""); }},
        file.raw(), mode.raw());
}
} // end namespace minilua::io