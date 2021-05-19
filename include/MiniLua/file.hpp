#ifndef MINILUA_FILE_HPP
#define MINILUA_FILE_HPP

#include <fstream>
#include <string>
#include <vector>

#include "MiniLua/values.hpp"

namespace minilua::file {

auto close(const CallContext& ctx) -> Value;
} // end namespace minilua::file

#endif