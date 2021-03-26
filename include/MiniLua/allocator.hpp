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

/**
 * @brief A memory allocator for the [Tables](@ref Table).
 *
 * It keeps track of all tables and can free them all at once.
 *
 * This was introduced to prevent memory leaks because tables can have cyclic
 * references. And really the environment always has a cyclic reference because
 * the global variable `_G` refers to the global environment. And additionally
 * function definitions capture the environment but are also stored in the
 * environment. So they form an indirect cycle.
 */
class MemoryAllocator {
    std::vector<TableImpl*> table_memory;

public:
    ~MemoryAllocator();

    /**
     * @brief Allocate an new table implementation object.
     *
     * This is used internally in Table.
     */
    auto allocate_table() -> TableImpl*;

    /**
     * @brief Returns the list of allocated tables.
     *
     * This is used by the interpreter to call the `__gc` metamethod on all
     * tables that have it.
     */
    [[nodiscard]] auto get_all() const -> const std::vector<TableImpl*>&;

    /**
     * @brief Free all objects created through this allocator.
     *
     * This assumes that there are no tables with a `__gc` method, or that all
     * of them have already been called.
     *
     * \warning This is highly unsafe! You have to be absolutely certain that
     * none of the values allocated by this will be used again.
     *
     * \warning **Any object/pointer allocated before calling this will become
     * invalid.**
     */
    void free_all();

    /**
     * @brief The number of allocated objects.
     */
    auto num_objects() -> std::size_t;
};

/**
 * @brief The global memory allocator.
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
