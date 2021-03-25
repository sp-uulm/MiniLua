#ifndef MINILUA_IO_HPP
#define MINILUA_IO_HPP

#include <iostream>
#include <istream>

#include "MiniLua/values.hpp"

namespace minilua {

auto create_io_table(MemoryAllocator* allocator) -> Table;

namespace io {

auto open(const CallContext& ctx) -> Vallist;

} // namespace io

} // namespace minilua

#endif
