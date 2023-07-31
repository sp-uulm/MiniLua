#ifndef MINILUA_UTILS_HPP
#define MINILUA_UTILS_HPP

#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace minilua {

/**
 * @brief Can be used in `std::enable_if` to check for printable types.
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
 * @brief Heap allocated owned value.
 *
 * This is basically an implementation of [Rusts
 * Box](https://doc.rust-lang.org/std/boxed/struct.Box.html). It behaves
 * exactly like `T` but lives on the heap.
 *
 * In other words this is a `std::unique_ptr` that also support copying but
 * can't contain a `nullptr`.
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
 * and equality comparable if the type `T` is. These special member functions
 * are always defined but can only be used if the type `T` also has them.
 *
 * Supports equality comparsion if `T` does.
 */
template <typename T> class owning_ptr {
    std::unique_ptr<T> value;

public:
    /**
     * @brief Default constructor.
     *
     * Only available if `T` is default constructible.
     */
    owning_ptr() : value(new T()) {
        static_assert(
            std::is_default_constructible_v<T>,
            "no default constructor for owning_ptr because T does not have one");
    }

    /**
     * @brief Creates an owning_ptr from the given pointer.
     *
     * \warning
     * This will take ownership of the heap location at the pointer. You should
     * not manually free the pointer after calling this constructor.
     *
     * Will throw `std::invalid_argument` exception if the value is `nullptr`.
     */
    explicit owning_ptr(T* value) : value(value) {
        if (value == nullptr) {
            throw std::invalid_argument("owning_ptr can't contain a nullptr");
        }
    }

    /**
     * @brief Copy constructor.
     *
     * This will make a new heap allocation.
     */
    owning_ptr(const owning_ptr<T>& other) : value(new T(*other.get())) {}

    /**
     * @brief Move constructor.
     *
     * This will make a new heap allocation and move the old value instead of
     * just moving the pointer. This is done so if you keep any pointers to the
     * old value around they will still point to the (now modified) old value.
     *
     * \note This can't be `noexcept` because it leads to errors when using the
     * constructor on some compilers.
     */
    owning_ptr(owning_ptr<T>&& other) : owning_ptr(new T(std::move(*other.value))) {}

    /**
     * @brief Copy assignment operator.
     *
     * This will make a new heap allocation.
     */
    auto operator=(const owning_ptr<T>& other) -> owning_ptr<T>& {
        value.reset(new T(*other.get()));
        return *this;
    }

    /**
     * @brief Move assignment operator.
     */
    auto operator=(owning_ptr<T>&& other) -> owning_ptr<T>& {
        *this->value = std::move(*other.value);
        return *this;
    }

    /**
     * @brief Swap function.
     */
    friend void swap(owning_ptr<T>& lhs, owning_ptr<T>& rhs) { std::swap(lhs.value, rhs.value); }

    /**
     * @brief Pointer to the heap value.
     */
    auto get() const noexcept -> typename std::unique_ptr<T>::pointer { return value.get(); }

    /**
     * @brief Derefernce to the owned type.
     *
     * Returns a reference to the owned type.
     */
    auto operator*() const -> T& { return value.operator*(); }
    /**
     * @brief Derefernce to the owned type.
     *
     * Returns a pointer to the owned type.
     */
    auto operator->() const noexcept -> typename std::unique_ptr<T>::pointer {
        return value.operator->();
    }

    /**
     * @brief Equality comparison.
     */
    friend auto operator==(const owning_ptr<T>& lhs, const owning_ptr<T>& rhs) -> bool {
        if (lhs.get() == nullptr || rhs.get() == nullptr) {
            throw std::runtime_error(
                "owning_ptr has become null by error (because copy/move/swap threw an error)");
        }

        return *lhs == *rhs;
    }
    /**
     * @brief Unequality comparison.
     */
    friend auto operator!=(const owning_ptr<T>& lhs, const owning_ptr<T>& rhs) -> bool {
        return !(lhs == rhs);
    }

    /**
     * @brief Print operator.
     *
     * Can always be printed. If `T` is not printable this will print the
     * pointer value (as a `void*`).
     */
    friend auto operator<<(std::ostream& os, const owning_ptr<T>& self) -> std::ostream& {
        if constexpr (is_printable_v<T>) {
            return os << "owning_ptr(" << *self.get() << ")";
        } else {
            return os << "owning_ptr(" << static_cast<void*>(self.get()) << ")";
        }
    }
};

/**
 * @breif Helper function to create owning_ptr.
 *
 * Similar to `std::make_unique`, etc.
 */
template <typename T, typename... Args> auto make_owning(Args... args) -> owning_ptr<T> {
    return owning_ptr<T>(new T(std::forward<Args>(args)...));
}

/**
 * @brief Overloading trick to make functions take multiple overloaded lambdas.
 *
 * Overloading trick for function that take lambdas where the parameter types
 * can be different.
 *
 * This is easier than writing
 *
 * ```cpp
 * if constexpr (std::is_same_v<decltype(param), T>) { ... }
 * ```
 *
 * inside the lambda.
 *
 * # Usage:
 *
 * ```cpp
 * std::visit(
 *     overloaded{
 *        [](int i) {
 *            std::cout << "This is an int " << i << "\n";
 *        },
 *        [](std::string s) {
 *            std::cout << "This is a string " << s << "\n";
 *        },
 *     },
 *     some_variant
 * );
 * ```
 *
 * Source: https://dev.to/tmr232/that-overloaded-trick-overloading-lambdas-in-c17
 */
template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

/**
 * @breif Convert int to string in the given base.
 *
 * Uses character 0-9 and A-Z. So base has to be between 2 and 36.
 */
auto to_string_with_base(int number, int base) -> std::string;

auto string_starts_with(const std::string& str, char ch) -> bool;

} // namespace minilua

#endif
