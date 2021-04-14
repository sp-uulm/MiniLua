#include "table.hpp"
#include <MiniLua/allocator.hpp>
#include <MiniLua/values.hpp>

namespace minilua {

// class MemoryAllocator
MemoryAllocator::~MemoryAllocator() { this->free_all(); }
auto MemoryAllocator::allocate_table() -> TableImpl* {
    auto* ptr = new TableImpl();
    table_memory.push_back(ptr);
    return ptr;
}
auto MemoryAllocator::get_all() const -> const std::vector<TableImpl*>& {
    return this->table_memory;
}

void MemoryAllocator::free_all() {
    for (TableImpl* ptr : table_memory) {
        delete ptr;
    }
    table_memory.clear();
}

auto MemoryAllocator::num_objects() -> std::size_t { return this->table_memory.size(); }

// NOTE: This WILL NOT prevent all memory leaks.
//
// This does not necessarily get destructed after everything else. That means
// if there are other global variables that allocate something in the
// GLOBAL_ALLOCATOR after this object gets destructed there WILL be a memory
// leak.
MemoryAllocator GLOBAL_ALLOCATOR;

} // namespace minilua
