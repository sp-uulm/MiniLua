#include "table.hpp"
#include <MiniLua/allocator.hpp>
#include <MiniLua/values.hpp>

namespace minilua {

// class MemoryAllocator
auto MemoryAllocator::allocate_table() -> TableImpl* {
    auto* ptr = new TableImpl();
    table_memory.push_back(ptr);
    return ptr;
}

void MemoryAllocator::free_all() {
    for (TableImpl* ptr : table_memory) {
        delete ptr;
    }
}

MemoryAllocator GLOBAL_ALLOCATOR;

// NOTE: This WILL NOT prevent all memory leaks.
//
// This does not necessarily get destructed after everything else. That means
// if there are other global variables that allocate something in the
// GLOBAL_ALLOCATOR after this object gets destructed there WILL be a memory
// leak.
class GlobalMemoryAllocatorCleanup {
public:
    ~GlobalMemoryAllocatorCleanup() { GLOBAL_ALLOCATOR.free_all(); }
};

static GlobalMemoryAllocatorCleanup _; // NOLINT

} // namespace minilua
