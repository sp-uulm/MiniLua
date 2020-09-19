#ifndef MINILUA_SOURCE_CHANGE_HPP
#define MINILUA_SOURCE_CHANGE_HPP

#include <cstdint>
#include <iostream>

namespace minilua {

/**
 * Represents a location in source code.
 *
 * NOTE: The comparison operators only consider the byte field. If you want
 * correct results you should only compare locations that were generated from
 * the same source code.
 */
struct Location {
    uint32_t line;
    uint32_t column;
    uint32_t byte;
};

constexpr auto operator==(Location lhs, Location rhs) noexcept -> bool {
    return lhs.byte == rhs.byte;
}
constexpr auto operator!=(Location lhs, Location rhs) noexcept -> bool {
    return !(lhs == rhs);
}
constexpr auto operator<(Location lhs, Location rhs) noexcept -> bool {
    return lhs.byte < rhs.byte;
}
constexpr auto operator<=(Location lhs, Location rhs) noexcept -> bool {
    return lhs.byte <= rhs.byte;
}
constexpr auto operator>(Location lhs, Location rhs) noexcept -> bool {
    return lhs.byte > rhs.byte;
}
constexpr auto operator>=(Location lhs, Location rhs) noexcept -> bool {
    return lhs.byte >= rhs.byte;
}
auto operator<<(std::ostream&, const Location&) -> std::ostream&;

struct Range {
    Location start;
    Location end;
};

constexpr auto operator==(Range lhs, Range rhs) noexcept -> bool {
    return lhs.start == rhs.start && lhs.end == rhs.end;
}
constexpr auto operator!=(Range lhs, Range rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream&, const Range&) -> std::ostream&;

struct SourceChange {
    Range range;
    std::string replacement;
};

auto operator==(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool;
auto operator!=(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SourceChange&) -> std::ostream&;

} // namespace minilua

#endif
