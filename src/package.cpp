#include "MiniLua/values.hpp"
#include <MiniLua/package.hpp>

namespace minilua {
auto create_package_table(MemoryAllocator* allocator) -> Table {
    std::unordered_map<Value, Value> math_functions;
    Table package(allocator);

    return package;
}

namespace package {

#ifndef _WIN64
Value config = "\\\n"
               ";\n"
               "?\n"
               "!\n"
               "-";
#else
minilua::Value config = "/\n"
                        ";\n"
                        "?\n"
                        "!\n"
                        "-";
#endif

Value cpath;
Table loaded;
Value path;
Value preload;
Value searchers;

} // end namespace package
} // end namespace minilua