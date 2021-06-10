#include "MiniLua/file.hpp"
#include "MiniLua/values.hpp"

namespace minilua::file {
std::fstream output;

auto close(const CallContext& ctx) -> Value {
    auto file = ctx.arguments().get(0);

    output.close();
    return true;
}

} // end namespace minilua::file