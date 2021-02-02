#include <catch2/catch.hpp>

#include <MiniLua/allocator.hpp>
#include <MiniLua/values.hpp>

namespace minilua {

TEST_CASE("raw MemoryAllocator usage") {
    MemoryAllocator alloc;

    auto* table1 = alloc.allocate_table();
    auto* table2 = alloc.allocate_table();

    REQUIRE(alloc.num_objects() == 2);
    CHECK(table1 != table2);

    alloc.free_all();
    REQUIRE(alloc.num_objects() == 0);
}

TEST_CASE("using MemoryAllocator with Table") {
    MemoryAllocator alloc;

    const Table table1(&alloc);
    REQUIRE(alloc.num_objects() == 1);
    const Table table2(&alloc);
    REQUIRE(alloc.num_objects() == 2);
    Table table3(table2);
    REQUIRE(alloc.num_objects() == 2);

    table3["key"] = table2;

    CHECK(table1 != table2);
    CHECK(table1 != table3);
    CHECK(table2 == table3);

    alloc.free_all();
    REQUIRE(alloc.num_objects() == 0);
}

} // namespace minilua
