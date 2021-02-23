#ifndef MINILUA_SOURCE_CHANGE_HPP
#define MINILUA_SOURCE_CHANGE_HPP

#include "MiniLua/utils.hpp"
#include <cstdint>
#include <iostream>
#include <memory>
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

    /**
     * Optional filename where the Range is located.
     *
     * The filename is behind a shared_ptr to avoid unnecessary copies.
     *
     * \note Changing the underlying string might lead to undefined behaviour.
     */
    std::optional<std::shared_ptr<std::string>> file;

    /**
     * Returns a copy of this range with the filename changed.
     */
    [[nodiscard]] auto with_file(std::optional<std::shared_ptr<std::string>> file) const -> Range;
};

auto operator==(Range lhs, Range rhs) noexcept -> bool;
auto operator!=(Range lhs, Range rhs) noexcept -> bool;
auto operator<<(std::ostream&, const Range&) -> std::ostream&;

class SourceChangeTree;

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
struct SourceChange : public CommonSCInfo {
    Range range;
    std::string replacement;

    SourceChange(Range range, std::string replacement);
};

auto operator==(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool;
auto operator!=(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SourceChange&) -> std::ostream&;

/**
 * Multiple source changes that all need to be applied together.
 */
struct SourceChangeCombination : public CommonSCInfo {
    std::vector<SourceChangeTree> changes;

    SourceChangeCombination();
    SourceChangeCombination(std::vector<SourceChangeTree> changes);

    void add(SourceChangeTree);
};

auto operator==(const SourceChangeCombination& lhs, const SourceChangeCombination& rhs) noexcept
    -> bool;
auto operator!=(const SourceChangeCombination& lhs, const SourceChangeCombination& rhs) noexcept
    -> bool;
auto operator<<(std::ostream&, const SourceChangeCombination&) -> std::ostream&;

/**
 * Multiple source changes where only one can be applied.
 */
struct SourceChangeAlternative : public CommonSCInfo {
    std::vector<SourceChangeTree> changes;

    SourceChangeAlternative();
    SourceChangeAlternative(std::vector<SourceChangeTree> changes);

    void add(SourceChangeTree);
    void add_if_some(std::optional<SourceChangeTree>);
};

auto operator==(const SourceChangeAlternative& lhs, const SourceChangeAlternative& rhs) noexcept
    -> bool;
auto operator!=(const SourceChangeAlternative& lhs, const SourceChangeAlternative& rhs) noexcept
    -> bool;
auto operator<<(std::ostream&, const SourceChangeAlternative&) -> std::ostream&;

/**
 * Wrapper for the source change tree.
 *
 * Walk the tree with `SourceChangeTree::visit`, `SourceChangeTree::visit_first_alternative`,
 * `SourceChangeTree::visit_all`.
 *
 * To simply get the first complete source change use `SourceChangeTree::collect_first_alternative`.
 */
class SourceChangeTree {
public:
    using Type = std::variant<SourceChange, SourceChangeCombination, SourceChangeAlternative>;

private:
    Type change;

public:
    SourceChangeTree(SourceChange);
    SourceChangeTree(SourceChangeCombination);
    SourceChangeTree(SourceChangeAlternative);
    SourceChangeTree(Type);

    /**
     * Returns the origin of the root source change.
     */
    [[nodiscard]] auto origin() const -> const std::string&;
    auto origin() -> std::string&;
    /**
     * Returns the hint of the root source change.
     */
    [[nodiscard]] auto hint() const -> const std::string&;
    auto hint() -> std::string&;

    /**
     * Removes the filename from the ranges in the tree.
     */
    void remove_filename();

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
        static_assert(std::is_invocable_v<Visitor, SourceChange&>);
        static_assert(std::is_invocable_v<Visitor, SourceChangeCombination&>);
        static_assert(std::is_invocable_v<Visitor, SourceChangeAlternative&>);
        return std::visit(visitor, change);
    }
    template <typename Visitor> decltype(auto) visit(Visitor visitor) const {
        static_assert(std::is_invocable_v<Visitor, SourceChange&>);
        static_assert(std::is_invocable_v<Visitor, SourceChangeCombination&>);
        static_assert(std::is_invocable_v<Visitor, SourceChangeAlternative&>);
        return std::visit(visitor, change);
    }

    // TODO this could be an iterator

    /**
     * Visits only the first child (left) of an or node. And nodes are completely visited.
     */
    template <typename Visitor> void visit_first_alternative(Visitor visitor) {
        static_assert(std::is_invocable_v<Visitor, SourceChange&>);
        this->visit(overloaded{
            [&visitor](SourceChange& leaf_node) { visitor(leaf_node); },
            [&visitor](SourceChangeCombination& and_node) {
                for (auto& change : and_node.changes) {
                    change.visit_first_alternative(visitor);
                }
            },
            [&visitor](SourceChangeAlternative& or_node) {
                if (!or_node.changes.empty()) {
                    or_node.changes[0].visit_first_alternative(visitor);
                }
            }});
    }
    template <typename Visitor> void visit_first_alternative(Visitor visitor) const {
        static_assert(std::is_invocable_v<Visitor, const SourceChange&>);
        this->visit(overloaded{
            [&visitor](const SourceChange& leaf_node) { visitor(leaf_node); },
            [&visitor](const SourceChangeCombination& and_node) {
                for (const auto& change : and_node.changes) {
                    change.visit_first_alternative(visitor);
                }
            },
            [&visitor](const SourceChangeAlternative& or_node) {
                if (!or_node.changes.empty()) {
                    or_node.changes[0].visit_first_alternative(visitor);
                }
            }});
    }

    /**
     * Visit all leaf nodes (SCSingle).
     */
    template <typename Visitor> void visit_all(Visitor visitor) {
        static_assert(std::is_invocable_v<Visitor, SourceChange&>);
        this->visit(overloaded{
            [&visitor](SourceChange& leaf_node) { visitor(leaf_node); },
            [&visitor](SourceChangeCombination& and_node) {
                for (auto& change : and_node.changes) {
                    change.visit_all(visitor);
                }
            },
            [&visitor](SourceChangeAlternative& or_node) {
                for (auto& change : or_node.changes) {
                    change.visit_all(visitor);
                }
            },
        });
    }
    template <typename Visitor> void visit_all(Visitor visitor) const {
        static_assert(std::is_invocable_v<Visitor, const SourceChange&>);
        this->visit(overloaded{
            [&visitor](const SourceChange& leaf_node) { visitor(leaf_node); },
            [&visitor](const SourceChangeCombination& and_node) {
                for (const auto& change : and_node.changes) {
                    change.visit_all(visitor);
                }
            },
            [&visitor](const SourceChangeAlternative& or_node) {
                for (const auto& change : or_node.changes) {
                    change.visit_all(visitor);
                }
            },
        });
    }

    /**
     * Collect only the left side of or-branches.
     */
    [[nodiscard]] auto collect_first_alternative() const -> std::vector<SourceChange>;

    // dereference to the underlying variant type
    auto operator*() -> Type&;
    auto operator*() const -> const Type&;
    auto operator->() -> Type*;
};

auto operator==(const SourceChangeTree& lhs, const SourceChangeTree& rhs) noexcept -> bool;
auto operator!=(const SourceChangeTree& lhs, const SourceChangeTree& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SourceChangeTree&) -> std::ostream&;

} // namespace minilua

#endif
