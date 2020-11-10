#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "val.hpp"

namespace lua {
namespace rt {

struct Environment : enable_shared_from_this<Environment> {
private:
    table t;
    shared_ptr<Environment> parent;
    table* global = nullptr;

public:
    Environment(const shared_ptr<Environment>& parent) : parent{parent} {
        if (parent) {
            global = parent->global;
        } else {
            global = &t;
        }
    }

    void clear() { t.clear(); }

    void assign(const val& var, const val& newval, bool is_local);
    val getvar(const val& var);

    void populate_stdlib();
};

} // namespace rt
} // namespace lua

#endif
