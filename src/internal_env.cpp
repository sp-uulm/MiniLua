#include "internal_env.hpp"
#include <MiniLua/allocator.hpp>

#include <utility>

namespace minilua {

// struct Environment::Impl
Environment::Impl::Impl(Env env) : inner(std::move(env)) {}
Environment::Impl::Impl(MemoryAllocator* allocator) : inner(Env(allocator)) {}

Env::Env() : Env(&GLOBAL_ALLOCATOR) {}
Env::Env(MemoryAllocator* allocator)
    : _allocator(allocator), in(&std::cin), out(&std::cout), err(&std::cerr) {}
Env::operator Environment() const { return Environment(Environment::Impl{*this}); }

auto Env::make_table() const -> Table { return Table(this->allocator()); }

auto Env::global() -> Table& { return this->_global; }
auto Env::global() const -> const Table& { return this->_global; }

auto Env::local() -> LocalEnv& { return this->_local; }
auto Env::local() const -> const LocalEnv& { return this->_local; }

void Env::declare_local(const std::string& name) {
    this->local().insert_or_assign(name, std::make_shared<Value>());
}
void Env::set_local(const std::string& name, Value value) {
    this->local().try_emplace(name, std::make_shared<Value>());
    *this->local().at(name) = std::move(value);
}
auto Env::get_local(const std::string& name) -> std::optional<Value> {
    if (this->local().find(name) == this->local().end()) {
        return std::nullopt;
    } else {
        return *this->local().at(name);
    }
}
auto Env::is_local(const std::string& name) -> bool {
    return this->local().find(name) != this->local().end();
}

void Env::set_global(const std::string& name, Value value) {
    this->global().set(name, std::move(value));
}
auto Env::get_global(const std::string& name) -> Value { return this->global().get(name); }

void Env::set_var(const std::string& name, Value value) {
    if (this->is_local(name)) {
        this->set_local(name, std::move(value));
    } else {
        this->set_global(name, std::move(value));
    }
}
auto Env::get_var(const std::string& name) -> Value {
    if (auto value = this->get_local(name)) {
        return *value;
    } else {
        return this->get_global(name);
    }
}
void Env::set_varargs(std::optional<Vallist> vallist) { this->varargs = std::move(vallist); }
auto Env::get_varargs() const -> std::optional<Vallist> { return this->varargs; }

void Env::set_stdin(std::istream* in) {
    if (in == nullptr) {
        throw std::invalid_argument("can't use nullptr as stdin");
    }
    this->in = in;
}
void Env::set_stdout(std::ostream* out) {
    if (out == nullptr) {
        throw std::invalid_argument("can't use nullptr as stdout");
    }
    this->out = out;
}
void Env::set_stderr(std::ostream* err) {
    if (err == nullptr) {
        throw std::invalid_argument("can't use nullptr as stderr");
    }
    this->err = err;
}

auto Env::get_stdin() -> std::istream* { return this->in; }
auto Env::get_stdout() -> std::ostream* { return this->out; }
auto Env::get_stderr() -> std::ostream* { return this->err; }

auto Env::allocator() const -> MemoryAllocator* { return this->_allocator; }

auto operator<<(std::ostream& os, const Env& self) -> std::ostream& {
    os << "Env{ .global = " << self.global() << ", .local = {";

    const auto* sep = "";
    for (const auto& [key, value] : self.local()) {
        os << sep << "\"" << key << "\": " << value;
        sep = ", ";
    }

    return os << "}\n";
}

}; // namespace minilua
