#ifndef MINILUA_HPP
#define MINILUA_HPP

#include "environment.hpp"
#include "interpreter.hpp"
#include "source_change.hpp"
#include "utils.hpp"
#include "values.hpp"

/**
 * This header defines the public api of the MiniLua library.
 *
 * We use the PImpl technique to hide implementation details (see below).
 *
 * TODO some more documentation on where to start and design decisions
 *
 * ---
 *
 * PImpl technique:
 *
 * Also see: https://en.cppreference.com/w/cpp/language/pimpl
 *
 * With the PImpl technique we hide all behaviour using a private nested forward
 * declaration of a struct or class. This has two important uses:
 *
 * 1. hiding the implementation details from the user
 * 2. breaking up cycles of incomplete types
 *    (this works because the cyclic reference are now in the cpp instead of the header)
 *
 * For this to work classes using the PImpl technique can't *use* the private nested
 * forward declaration. This means we have to declare but not define the methods
 * in the header. This includes special member functions like copy-constructor and
 * destructor. Then these methods have to be implemented in the cpp file and the
 * only difference is basically instead of using `this->` they have to use
 * `this->impl->` or just `impl->`.
 *
 * A class with the PImpl technique usually looks like this:
 *
 * ```
 * // in the header
 * class Something {
 *   struct Impl;
 *   owning_ptr<Impl> impl; // any pointer type is ok here
 *
 * public:
 *   // normal constructor declarations
 *   Something(const Something&);
 *   Something(Something&&) noexcept;
 *   Something& operator=(const Something& other);
 *   Something& operator=(Something&& other);
 *   friend void swap(Something& self, Something& other);
 * };
 *
 * // in the cpp
 * struct Something::Impl {
 *   // fields you would have put in class Something
 * };
 * Something::Something(const Something&) = default;
 * Something(Something&&) noexcept = default;
 * Something& operator=(const Something& other) = default;
 * Something& operator=(Something&& other) = default;
 * void swap(Something& self, Something& other) {
 *   std::swap(self.impl, other.impl);
 * }
 * ```
 *
 * If the nested `struct Impl` is not default constructible you have to provide
 * the implementation of the copy-/move-constructors and -operators manually and
 * can't use `= default`.
 *
 * `owning_ptr<T>` was chosen for the pointer type because it behaves like a
 * normal `T` (but lives on the heap). It's move, copy and lifetime semantics
 * are identical to the one of `T`.
 *
 * Is is also possible to choose another pointer type like `std::unique_ptr` or
 * `std::shared_ptr`. You should probably avoid using raw pointer `T*` because
 * lifetime management is harder with raw pointers.
 */
namespace minilua {} // namespace minilua

#endif
