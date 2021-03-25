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
 * @brief A location in source code.
 *
 * \note The comparison operators only consider the byte field. You should only
 * compare locations that were generated from the same source code.
 *
 * Supports the comparison operators.
 */
struct Location {
    /**
     * @brief The line.
     */
    uint32_t line;
    /**
     * @brief The column.
     */
    uint32_t column;
    /**
     * @brief The absolute byte offset.
     */
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
 * @brief A range (sometimes called span) in source code.
 *
 * Supports equality operators.
 */
struct Range {
    /**
     * @brief Start of the range.
     */
    Location start;
    /**
     * @brief End of the range (exclusive).
     */
    Location end;

    /**
     * Optional filename where the Range is located.
     *
     * The filename is behind a shared_ptr to avoid unnecessary copies.
     */
    std::optional<std::shared_ptr<const std::string>> file;

    /**
     * Returns a copy of this range with the filename changed.
     */
    [[nodiscard]] auto with_file(std::optional<std::shared_ptr<const std::string>> file) const
        -> Range;
};

auto operator==(const Range& lhs, const Range& rhs) noexcept -> bool;
auto operator!=(const Range& lhs, const Range& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const Range&) -> std::ostream&;

class SourceChangeTree;

/**
 * @brief Common information for source changes.
 */
struct CommonSCInfo {
    /**
     * @brief Can be filled in by the function creating the suggestion.
     */
    std::string origin;
    /**
     * @brief Hint for the source locations that would be modified (e.g.
     * variable name/line number).
     */
    std::string hint;
};

/**
 * @brief A source change for a single location.
 *
 * Supports equality operators.
 */
struct SourceChange : public CommonSCInfo {
    /**
     * @brief The range to replace.
     */
    Range range;
    /**
     * The replacement.
     */
    std::string replacement;

    /**
     * Create a single SourceChange with empty origin and hint.
     */
    SourceChange(Range range, std::string replacement);

    /**
     * Only here for convenience. Simply returns a copy of this object.
     */
    [[nodiscard]] auto simplify() const -> SourceChange;
};

auto operator==(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool;
auto operator!=(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SourceChange&) -> std::ostream&;

/**
 * @brief Multiple source changes that all need to be applied together (i.e. in
 * combination).
 *
 * Supports equality operators.
 */
struct SourceChangeCombination : public CommonSCInfo {
    /**
     * @brief The changes to apply together.
     */
    std::vector<SourceChangeTree> changes;

    /**
     * @brief Default constructor.
     */
    SourceChangeCombination();
    /**
     * @brief Constructor with data.
     */
    SourceChangeCombination(std::vector<SourceChangeTree> changes);

    /**
     * @brief Add any source change to the combination.
     */
    void add(SourceChangeTree);

    /**
     * (Recursively) simplifies the tree.
     *
     * Empty combinations will be converted to nullopts.
     */
    [[nodiscard]] auto simplify() const -> std::optional<SourceChangeTree>;
};

auto operator==(const SourceChangeCombination& lhs, const SourceChangeCombination& rhs) noexcept
    -> bool;
auto operator!=(const SourceChangeCombination& lhs, const SourceChangeCombination& rhs) noexcept
    -> bool;
auto operator<<(std::ostream&, const SourceChangeCombination&) -> std::ostream&;

/**
 * @brief Multiple source changes where only one can be applied (i.e.
 * alternatives).
 *
 * Supports equality operators.
 */
struct SourceChangeAlternative : public CommonSCInfo {
    /**
     * @brief The alternatives.
     */
    std::vector<SourceChangeTree> changes;

    /**
     * @brief Default constructor.
     */
    SourceChangeAlternative();
    /**
     * @brief Constructor with data.
     */
    SourceChangeAlternative(std::vector<SourceChangeTree> changes);

    /**
     * @brief Add any source change to the alternative.
     */
    void add(SourceChangeTree);
    /**
     * @brief Only add the source change if it is not `std::nullopt`.
     */
    void add_if_some(std::optional<SourceChangeTree>);

    /**
     * (Recursively) simplifies the tree.
     *
     * Empty alternatives will be converted to nullopts.
     */
    [[nodiscard]] auto simplify() const -> std::optional<SourceChangeTree>;
};

auto operator==(const SourceChangeAlternative& lhs, const SourceChangeAlternative& rhs) noexcept
    -> bool;
auto operator!=(const SourceChangeAlternative& lhs, const SourceChangeAlternative& rhs) noexcept
    -> bool;
auto operator<<(std::ostream&, const SourceChangeAlternative&) -> std::ostream&;

/**
 * @brief Wrapper for the source change tree.
 *
 * Walk the tree with `SourceChangeTree::visit`,
 * `SourceChangeTree::visit_first_alternative`, `SourceChangeTree::visit_all`.
 *
 * To simply get the first complete source change use
 * `SourceChangeTree::collect_first_alternative`.
 *
 * Supports equality operators.
 */
class SourceChangeTree {
public:
    using Type = std::variant<SourceChange, SourceChangeCombination, SourceChangeAlternative>;

private:
    Type change;

public:
    /**
     * @brief Constructor taking a single `SourceChange`.
     */
    SourceChangeTree(SourceChange);
    /**
     * @brief Constructor taking a `SourceChangeCombination`.
     */
    SourceChangeTree(SourceChangeCombination);
    /**
     * @brief Constructor taking a `SourceChangeAlternative`.
     */
    SourceChangeTree(SourceChangeAlternative);
    /**
     * @brief Constructor taking the internal `std::variant`.
     */
    SourceChangeTree(Type);

    /**
     * @brief The origin of the root source change.
     */
    [[nodiscard]] auto origin() const -> const std::string&;
    /**
     * @brief The origin of the root source change.
     */
    auto origin() -> std::string&;
    /**
     * @brief The hint of the root source change.
     */
    [[nodiscard]] auto hint() const -> const std::string&;
    /**
     * @brief The hint of the root source change.
     */
    auto hint() -> std::string&;

    /**
     * Removes the filename from the ranges in the tree.
     */
    void remove_filename();

    /**
     * @brief Visit the root node of the tree of source changes.
     *
     * \note You have to manually navigate through the tree.
     *
     * `Visitor` has to be callable with `SourceChange&`,
     * `SourceChangeCombination&` and `SourceChangeAlternative&`.
     *
     * For possible implementations see
     * `SourceChangeTree::visit_first_alternative`.
     */
    template <typename Visitor> decltype(auto) visit(Visitor visitor) {
        static_assert(std::is_invocable_v<Visitor, SourceChange&>);
        static_assert(std::is_invocable_v<Visitor, SourceChangeCombination&>);
        static_assert(std::is_invocable_v<Visitor, SourceChangeAlternative&>);
        return std::visit(visitor, change);
    }
    /**
     * @brief Visit the root node of the tree of source changes.
     *
     * \note You have to manually navigate through the tree.
     *
     * `Visitor` has to be callable with `const SourceChange&`,
     * `const SourceChangeCombination&` and `const SourceChangeAlternative&`.
     *
     * For possible usage see implementation of
     * `SourceChangeTree::visit_first_alternative`.
     */
    template <typename Visitor> decltype(auto) visit(Visitor visitor) const {
        static_assert(std::is_invocable_v<Visitor, const SourceChange&>);
        static_assert(std::is_invocable_v<Visitor, const SourceChangeCombination&>);
        static_assert(std::is_invocable_v<Visitor, const SourceChangeAlternative&>);
        return std::visit(visitor, change);
    }

    // TODO this could be an iterator

    /**
     * @brief Visits only the first child (left) of an SourceChangeAlternative
     * node.
     *
     * SourceChangeCombination nodes are completely visited.
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
    /**
     * @brief Visits only the first child (left) of an SourceChangeAlternative
     * node.
     *
     * SourceChangeCombination nodes are completely visited.
     */
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
     * @brief Visit all leaf SourceChange nodes.
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
    /**
     * @brief Visit all leaf SourceChange nodes.
     */
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
     * @brief Collect only the left side of SourceChangeAlternative branches.
     */
    [[nodiscard]] auto collect_first_alternative() const -> std::vector<SourceChange>;

    /**
     * Simplify the source change tree removing all redundant nodes.
     *
     * This is recursive.
     *
     * Empty alternatives and combinations will be converted to nullopts.
     */
    [[nodiscard]] auto simplify() const -> std::optional<SourceChangeTree>;

    /**
     * @brief Derefernce to the underlying variant type.
     *
     * Returns a reference to the variant type.
     */
    auto operator*() -> Type&;
    /**
     * @brief Derefernce to the underlying variant type.
     *
     * Returns a reference to the variant type.
     */
    auto operator*() const -> const Type&;
    /**
     * @brief Derefernce to the underlying variant type.
     *
     * Returns a pointer to the variant type.
     */
    auto operator->() -> Type*;
};

auto operator==(const SourceChangeTree& lhs, const SourceChangeTree& rhs) noexcept -> bool;
auto operator!=(const SourceChangeTree& lhs, const SourceChangeTree& rhs) noexcept -> bool;
auto operator<<(std::ostream&, const SourceChangeTree&) -> std::ostream&;
auto operator<<(std::ostream&, const std::optional<SourceChangeTree>&) -> std::ostream&;

/**
 * @brief See SourceChangeTree::simplify.
 */
auto simplify(const std::optional<SourceChangeTree>& tree) -> std::optional<SourceChangeTree>;

/**
 * @brief Combines two source changes using a `SourceChangeCombination` if necessary.
 */
auto combine_source_changes(
    const std::optional<SourceChangeTree>& lhs, const std::optional<SourceChangeTree>& rhs)
    -> std::optional<SourceChangeTree>;

} // namespace minilua

#endif
