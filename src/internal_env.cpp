#include "internal_env.hpp"

#include <utility>

namespace minilua {

Env::Env() : in(&std::cin), out(&std::cout), err(&std::cerr) {}

auto Env::global() -> Table& { return this->_global; }
auto Env::global() const -> const Table& { return this->_global; }

auto Env::local() -> LocalEnv& { return this->_local; }
auto Env::local() const -> const LocalEnv& { return this->_local; }

void Env::declare_local(const std::string& name) {
    this->_local.insert_or_assign(name, std::make_shared<Value>());
}
void Env::set_local(const std::string& name, Value value) {
    this->_local.try_emplace(name, std::make_shared<Value>());
    *this->_local.at(name) = std::move(value);
}
auto Env::get_local(const std::string& name) -> std::optional<Value> {
    if (this->_local.find(name) == this->_local.end()) {
        return std::nullopt;
    } else {
        return *this->_local.at(name);
    }
}

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

}; // namespace minilua