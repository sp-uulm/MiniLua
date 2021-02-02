#ifndef MINILUA_ALLOCATOR_H
#define MINILUA_ALLOCATOR_H

#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace minilua {

struct TableImpl;

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
    /**
     * This will allocate an new table implementation object.
     *
     * This is used internally in `Table`.
     */
    auto allocate_table() -> TableImpl*;

    /**
     * This will free all objects created through this allocator.
     *
     * This is highly unsafe! You have to be absolutely certain that none of the
     * values allocated by this will be used again.
     *
     * **Any object/pointer allocated before calling this will become invalid.**
     */
    void free_all();

    /**
     * Return the number of allocated objects.
     */
    auto num_objects() -> std::size_t;
};

/**
 * A global memory allocator.
 *
 * This is meant only for use for values outside of the interpreter and outside
 * of `Function`s.
 *
 * This will get freed when the program terminates. You can also manually free
 * it but you need to be **absolutely certain** that none of the values
 * allocated with it are not in use anymore.
 */
extern MemoryAllocator GLOBAL_ALLOCATOR;

} // namespace minilua

#endif
