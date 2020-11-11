#ifndef MINILUA_SOURCE_CHANGE_HPP
#define MINILUA_SOURCE_CHANGE_HPP

#include <cstdint>
#include <iostream>
#include <optional>
#include <variant>
#include <vector>

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
constexpr auto operator!=(Location lhs, Location rhs) noexcept -> bool { return !(lhs == rhs); }
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
constexpr auto operator!=(Range lhs, Range rhs) noexcept -> bool { return !(lhs == rhs); }
auto operator<<(std::ostream&, const Range&) -> std::ostream&;

class SourceChange;

struct CommonSCInfo {
    // can be filled in by the function creating the suggestion
    std::string origin;
    // hint for the source locations that would be modified (e.g. variable name/line number)
    std::string hint;
};

struct SCSingle : public CommonSCInfo {
    Range range;
    std::string replacement;

    SCSingle();
    SCSingle(Range range, std::string replacement);
};

auto operator==(const SCSingle& lhs, const SCSingle& rhs) noexcept -> bool;
auto operator!=(const SCSingle& lhs, const SCSingle& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SCSingle&) -> std::ostream&;

struct SCAnd : public CommonSCInfo {
    std::vector<SourceChange> changes;

    SCAnd();
    SCAnd(std::vector<SourceChange> changes);

    void add(SourceChange);
};

auto operator==(const SCAnd& lhs, const SCAnd& rhs) noexcept -> bool;
auto operator!=(const SCAnd& lhs, const SCAnd& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SCAnd&) -> std::ostream&;

struct SCOr : public CommonSCInfo {
    std::vector<SourceChange> changes;

    SCOr();
    SCOr(std::vector<SourceChange> changes);

    void add(SourceChange);
};

auto operator==(const SCOr& lhs, const SCOr& rhs) noexcept -> bool;
auto operator!=(const SCOr& lhs, const SCOr& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SCOr&) -> std::ostream&;

// can't be just "using SourceChange = ..." because we need a forward reference above
class SourceChange {
public:
    using Type = std::variant<SCSingle, SCAnd, SCOr>;
    Type change;

    SourceChange();
    SourceChange(SCSingle);
    SourceChange(SCAnd);
    SourceChange(SCOr);
    SourceChange(Type);

    [[nodiscard]] auto origin() const -> const std::string&;
    [[nodiscard]] auto hint() const -> const std::string&;
    void set_origin(std::string);
    void set_hint(std::string);

    template <typename Visitor> decltype(auto) visit(Visitor visitor) {
        return std::visit(visitor, change);
    }
    template <typename Visitor> decltype(auto) visit(Visitor visitor) const {
        return std::visit(visitor, change);
    }

    auto operator*() -> Type&;
    auto operator*() const -> const Type&;

    auto operator->() -> Type*;
};

auto operator==(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool;
auto operator!=(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SourceChange&) -> std::ostream&;

} // namespace minilua

#endif
