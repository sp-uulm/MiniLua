#include "MiniLua/environment.hpp"

#include <string>

#include "MiniLua/values.hpp"
#include "internal_env.hpp"

namespace minilua {

Environment::Environment() : impl(make_owning<Impl>()) {}

Environment::Environment(const Environment&) = default;
// NOLINTNEXTLINE
Environment::Environment(Environment&&) = default;
Environment::~Environment() = default;
auto Environment::operator=(const Environment&) -> Environment& = default;
// NOLINTNEXTLINE
auto Environment::operator=(Environment&&) -> Environment& = default;
void swap(Environment& a, Environment& b) { swap(a.impl, b.impl); }

void Environment::add_default_stdlib() {
    // TODO add actual stdlib
}
void Environment::add(const std::string& name, Value value) {
    impl->inner.global().insert_or_assign(name, value);
}
void Environment::add(std::string&& name, Value value) {
    impl->inner.global().insert_or_assign(std::move(name), value);
}

void Environment::add_all(std::unordered_map<std::string, Value> values) {
    impl->inner.global().insert(values.begin(), values.end());
}
void Environment::add_all(std::initializer_list<std::pair<const std::string, Value>> values) {
    impl->inner.global().insert(values);
}

auto Environment::get(const std::string& name) -> Value& { return impl->inner.global().at(name); }

void Environment::set_stdin(std::istream* in) { impl->inner.set_stdin(in); }
void Environment::set_stdout(std::ostream* out) { impl->inner.set_stdout(out); }
void Environment::set_stderr(std::ostream* err) { impl->inner.set_stderr(err); }

auto Environment::get_stdin() -> std::istream* { return impl->inner.get_stdin(); }
auto Environment::get_stdout() -> std::ostream* { return impl->inner.get_stdout(); }
auto Environment::get_stderr() -> std::ostream* { return impl->inner.get_stderr(); }

auto Environment::size() const -> size_t { return impl->inner.global().size(); }

auto operator==(const Environment& a, const Environment& b) noexcept -> bool {
    return a.impl->inner.global() == b.impl->inner.global();
}
auto operator!=(const Environment& a, const Environment& b) noexcept -> bool { return !(a == b); }
auto operator<<(std::ostream& os, const Environment& self) -> std::ostream& {
    os << "Environment{";
    for (const auto& [key, value] : self.impl->inner.global()) {
        os << "[\"" << key << "\"] = " << value << ", ";
    }
    return os << "}";
}

}; // namespace minilua
