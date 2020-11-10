#ifndef MINILUA_UTILS_HPP
#define MINILUA_UTILS_HPP

#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace minilua {

/**
 * Can be used in std::enable_if to check for printable types.
 *
 * Source: https://nafe.es/posts/2020-02-29-is-printable/
 */
template <typename _Tp, typename dummy = void> struct is_printable : std::false_type {};

template <typename _Tp>
struct is_printable<
    _Tp, typename std::enable_if_t<
             std::is_same_v<decltype(std::cout << std::declval<_Tp>()), std::ostream&>>>
    : std::true_type {};

template <typename _Tp> inline constexpr bool is_printable_v = is_printable<_Tp>::value;

/**
 * This is basically an implementation of Rusts Box. It behaves exactly like T
 * but lives on the heap.
 *
 * In other words this is a `std::unique_ptr` that also support copying but can't
 * contain a nullptr.
 *
 * If this type is copied it will make a new heap allocation and copy the value.
 *
 * The default constructor of this class will call the default constructor of
 * `T` instead of using a nullptr (like `std::unique_ptr` does).
 *
 * In case any of the copy/move/swap functions throw an exception you should
 * expect the owning_ptr to be in an invalid state and to possibly contain a
 * nullptr.
 *
 * owning_ptr is only default constructible, move/copy constructible/assignable
 * and equality comparable if the type T is. These special member functions are
 * always defined but can only be used if the type T also has them.
 */
template <typename T> class owning_ptr {
    std::unique_ptr<T> value;

public:
    owning_ptr() : value(new T()) {
        static_assert(
            std::is_default_constructible_v<T>,
            "no default constructor for owning_ptr because T does not have one");
    }

    explicit owning_ptr(T* value) : value(value) {
        if (value == nullptr) {
            throw std::invalid_argument("owning_ptr can't contain a nullptr");
        }
    }

    owning_ptr(const owning_ptr<T>& other) : value(new T(*other.get())) {}

    owning_ptr(owning_ptr<T>&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : owning_ptr(new T(std::move(*other.value))) {}

    auto operator=(const owning_ptr<T>& other) -> owning_ptr<T>& {
        value.reset(new T(*other.get()));
        return *this;
    }

    auto operator=(owning_ptr<T>&& other) noexcept(std::is_nothrow_move_assignable_v<T>)
        -> owning_ptr<T>& {
        *this->value = std::move(*other.value);
        return *this;
    }

    friend void swap(owning_ptr<T>& lhs, owning_ptr<T>& rhs) { std::swap(lhs.value, rhs.value); }

    auto get() const noexcept -> typename std::unique_ptr<T>::pointer { return value.get(); }

    auto operator*() const -> T& { return value.operator*(); }
    auto operator->() const noexcept -> typename std::unique_ptr<T>::pointer {
        return value.operator->();
    }

    friend auto operator==(const owning_ptr<T>& lhs, const owning_ptr<T>& rhs) -> bool {
        if (lhs.get() == nullptr) {
            // can' derefence nullptr
            return rhs.get() == nullptr;
        } else if (rhs.get() == nullptr) {
            return false;
        }

        return *lhs == *rhs;
    }
    friend auto operator!=(const owning_ptr<T>& lhs, const owning_ptr<T>& rhs) -> bool {
        return !(lhs == rhs);
    }

    friend auto operator<<(std::ostream& os, const owning_ptr<T>& self) -> std::ostream& {
        if constexpr (is_printable_v<T>) {
            return os << "owning_ptr(" << *self.get() << ")";
        } else {
            return os << "owning_ptr(" << static_cast<void*>(self.get()) << ")";
        }
    }
};

template <typename T, typename... Args> auto make_owning(Args... args) -> owning_ptr<T> {
    return owning_ptr<T>(new T(std::forward<Args>(args)...));
}

// Overloading trick for lambdas
// This is easier than
//   if constexpr (std::is_same_v<decltype(param), T>) { ... }
// Source: https://dev.to/tmr232/that-overloaded-trick-overloading-lambdas-in-c17
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

} // namespace minilua

#endif
