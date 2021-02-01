#include "gc.hpp"

namespace minilua::details {

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

} // namespace minilua::details
