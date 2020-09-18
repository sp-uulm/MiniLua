#include <memory>
#include <type_traits>

namespace minilua {

/**
 * This type behaves exactly like the type `T` it wraps but `T` is allocated on
 * the heap.
 *
 * In other words this is a `std::unique_ptr` that also support copying.
 *
 * If this type is copied it will make a new heap allocation and copy the value.
 *
 * The default constructor of this class will call the default constructor of
 * `T` instead of using a nullptr (like `std::unique_ptr` does).
 */
template <typename T> class owning_ptr : public std::unique_ptr<T> {
public:
    using std::unique_ptr<T>::unique_ptr;

    owning_ptr() {
        static_assert(
            std::is_default_constructible_v<T>,
            "owning_ptr only has a default constructor if T has one");
        this->reset(new T());
    }

    owning_ptr(const owning_ptr<T>& other) {
        this->reset(new T(*other.get()));
    }

    auto operator=(const owning_ptr<T>& other) -> owning_ptr<T>& {
        this->reset(new T(*other.get()));
        return *this;
    }
};

template <typename T, typename... Args> auto make_owning(Args... args) -> owning_ptr<T> {
    return owning_ptr<T>(new T(std::forward<Args>(args)...));
}

} // namespace minilua
