#include <catch2/catch.hpp>

#include "details/gc.hpp"

namespace minilua::details {

TEST_CASE("gc") {
    MemoryAllocator alloc;

    auto* table1 = alloc.allocate_table();
    auto* table2 = alloc.allocate_table();

    CHECK(table1 != table2);

    alloc.free_all();
}

} // namespace minilua::details
