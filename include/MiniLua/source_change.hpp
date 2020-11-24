#ifndef MINILUA_SOURCE_CHANGE_HPP
#define MINILUA_SOURCE_CHANGE_HPP

#include "MiniLua/utils.hpp"
#include <cstdint>
#include <iostream>
#include <numeric>
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

/**
 * Represents a range/span in source code.
 */
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

// common information for source changes
struct CommonSCInfo {
    // can be filled in by the function creating the suggestion
    std::string origin;
    // hint for the source locations that would be modified (e.g. variable name/line number)
    std::string hint;
};

/**
 * A source change for a single location.
 */
struct SCSingle : public CommonSCInfo {
    Range range;
    std::string replacement;

    SCSingle(Range range, std::string replacement);
};

auto operator==(const SCSingle& lhs, const SCSingle& rhs) noexcept -> bool;
auto operator!=(const SCSingle& lhs, const SCSingle& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SCSingle&) -> std::ostream&;

/**
 * Multiple source changes that has to all be applied together.
 */
struct SCAnd : public CommonSCInfo {
    std::vector<SourceChange> changes;

    SCAnd();
    SCAnd(std::vector<SourceChange> changes);

    void add(SourceChange);
};

auto operator==(const SCAnd& lhs, const SCAnd& rhs) noexcept -> bool;
auto operator!=(const SCAnd& lhs, const SCAnd& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SCAnd&) -> std::ostream&;

/**
 * Multiple source changes where only one can be applied.
 */
struct SCOr : public CommonSCInfo {
    std::vector<SourceChange> changes;

    SCOr();
    SCOr(std::vector<SourceChange> changes);

    void add(SourceChange);
};

auto operator==(const SCOr& lhs, const SCOr& rhs) noexcept -> bool;
auto operator!=(const SCOr& lhs, const SCOr& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SCOr&) -> std::ostream&;

/**
 * Wrapper for a source change tree.
 */
class SourceChange {
public:
    using Type = std::variant<SCSingle, SCAnd, SCOr>;
    Type change;

    SourceChange(SCSingle);
    SourceChange(SCAnd);
    SourceChange(SCOr);
    SourceChange(Type);

    /**
     * Returns the origin of the root source change.
     */
    [[nodiscard]] auto origin() const -> const std::string&;
    /**
     * Returns the hint of the root source change.
     */
    [[nodiscard]] auto hint() const -> const std::string&;

    /**
     * Set the origin of the root source change.
     */
    void set_origin(std::string);
    /**
     * Set the hint of the root source change.
     */
    void set_hint(std::string);

    /**
     * Visit the root node of the tree of source changes.
     *
     * You have to manually navigate the tree.
     *
     * Visitor has to be callable with SCSingle&, SCAnd&, SCOr& (or with const
     * references for the const version of the method).
     *
     * For possible implementations see visit_left.
     */
    template <typename Visitor> decltype(auto) visit(Visitor visitor) {
        return std::visit(visitor, change);
    }
    template <typename Visitor> decltype(auto) visit(Visitor visitor) const {
        return std::visit(visitor, change);
    }

    // TODO this could be an iterator

    /**
     * Visits only the first child (left) of an or node. And nodes are completely visited.
     */
    template <typename Visitor> void visit_left(Visitor visitor) {
        this->visit(overloaded{
            [&visitor](SCSingle& leaf_node) { visitor(leaf_node); },
            [&visitor](SCAnd& and_node) {
                for (auto& change : and_node.changes) {
                    change.visit_left(visitor);
                }
            },
            [&visitor](SCOr& or_node) {
                if (!or_node.changes.empty()) {
                    or_node.changes[0].visit_left(visitor);
                }
            }});
    }
    template <typename Visitor> void visit_left(Visitor visitor) const {
        this->visit(overloaded{
            [&visitor](const SCSingle& leaf_node) { visitor(leaf_node); },
            [&visitor](const SCAnd& and_node) {
                for (const auto& change : and_node.changes) {
                    change.visit_left(visitor);
                }
            },
            [&visitor](const SCOr& or_node) {
                if (!or_node.changes.empty()) {
                    or_node.changes[0].visit_left(visitor);
                }
            }});
    }

    /**
     * Visit all leaf nodes (SCSingle).
     */
    template <typename Visitor> void visit_all(Visitor visitor) {
        this->visit(overloaded{
            [&visitor](SCSingle& leaf_node) { visitor(leaf_node); },
            [&visitor](SCAnd& and_node) {
                for (auto& change : and_node.changes) {
                    change.visit_left(visitor);
                }
            },
            [&visitor](SCOr& or_node) {
                for (auto& change : or_node.changes) {
                    change.visit_left(visitor);
                }
            },
        });
    }
    template <typename Visitor> void visit_all(Visitor visitor) const {
        this->visit(overloaded{
            [&visitor](const SCSingle& leaf_node) { visitor(leaf_node); },
            [&visitor](const SCAnd& and_node) {
                for (const auto& change : and_node.changes) {
                    change.visit_left(visitor);
                }
            },
            [&visitor](const SCOr& or_node) {
                for (const auto& change : or_node.changes) {
                    change.visit_left(visitor);
                }
            },
        });
    }

    /**
     * Collect only the left side of or-branches.
     */
    [[nodiscard]] auto collect_left() const -> std::vector<SCSingle>;

    // dereference to the underlying variant type
    auto operator*() -> Type&;
    auto operator*() const -> const Type&;

    auto operator->() -> Type*;
};

auto operator==(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool;
auto operator!=(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SourceChange&) -> std::ostream&;

} // namespace minilua

#endif
