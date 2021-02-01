#ifndef MINILUA_DETAILS_GC_H
#define MINILUA_DETAILS_GC_H

#include "../table.hpp"

#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace minilua::details {

template <typename T> class gc_ptr {
    T* ptr;

public:
    gc_ptr(T* ptr) : ptr(ptr) {}

    auto operator*() const -> T& { return *ptr; }
    auto operator->() const noexcept -> T* { return ptr; }

    friend auto operator==(const gc_ptr<T>& lhs, const gc_ptr<T>& rhs) -> bool {
        return lhs.ptr == rhs.ptr;
    }
    friend auto operator!=(const gc_ptr<T>& lhs, const gc_ptr<T>& rhs) -> bool {
        return !(lhs.ptr == rhs.ptr);
    }
};

template <typename T> gc_ptr(T*) -> gc_ptr<T>;

class MemoryAllocator {
    std::vector<Table::Impl*> table_memory;

public:
    auto allocate_table() -> Table::Impl* {
        auto* ptr = new Table::Impl();
        table_memory.push_back(ptr);
        return ptr;
    }

    /**
     * This is highly unsafe! Any gc_ptr will become invalid.
     */
    void free_all() {
        for (Table::Impl* ptr : table_memory) {
            delete ptr;
        }
    }
};

extern MemoryAllocator GLOBAL_ALLOCATOR;

} // namespace minilua::details

#endif
