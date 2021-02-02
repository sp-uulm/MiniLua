#ifndef MINILUA_ALLOCATOR_H
#define MINILUA_ALLOCATOR_H

#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace minilua {

class TableImpl;

// template <typename T> class gc_ptr {
//     T* ptr;
//
// public:
//     gc_ptr(T* ptr) : ptr(ptr) {}
//
//     auto operator*() const -> T& { return *ptr; }
//     auto operator->() const noexcept -> T* { return ptr; }
//
//     friend auto operator==(const gc_ptr<T>& lhs, const gc_ptr<T>& rhs) -> bool {
//         return lhs.ptr == rhs.ptr;
//     }
//     friend auto operator!=(const gc_ptr<T>& lhs, const gc_ptr<T>& rhs) -> bool {
//         return !(lhs.ptr == rhs.ptr);
//     }
// };
//
// template <typename T> gc_ptr(T*) -> gc_ptr<T>;

class MemoryAllocator {
    std::vector<TableImpl*> table_memory;

public:
    auto allocate_table() -> TableImpl*;

    /**
     * This is highly unsafe! Any gc_ptr will become invalid.
     */
    void free_all();
};

extern MemoryAllocator GLOBAL_ALLOCATOR;

} // namespace minilua

#endif
