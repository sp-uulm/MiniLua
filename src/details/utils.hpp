#ifndef MINILUA_DETAILS_UTIL_HPP
#define MINILUA_DETAILS_UTIL_HPP

#include <bits/stdint-uintn.h>

namespace minilua {

// Source:
// https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key/57556517#57556517

inline auto xorshift(const uint64_t& n, int i) -> uint64_t { return n ^ (n >> i); }
inline auto hash(const uint64_t& n) -> uint64_t {
    // pattern of alternating 0 and 1
    uint64_t p = 0x5555555555555555ull; // NOLINT
    // random uneven integer constant
    uint64_t c = 17316035218449499591ull; // NOLINT

    return c * xorshift(p * xorshift(n, 32), 32); // NOLINT
}

} // namespace minilua

#endif
