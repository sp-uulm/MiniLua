#ifndef TREE_SITTER_HPP
#define TREE_SITTER_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tree_sitter/api.h>
#include <utility>

extern "C" const TSLanguage* tree_sitter_lua();

/// Wrapper types and helper functions for Tree-Sitter.
namespace ts {

/**
 * Contains a source location.
 */
struct Point {
    std::uint32_t row;
    std::uint32_t column;
};

bool operator==(const Point&, const Point&);
std::ostream& operator<<(std::ostream&, const Point&);

struct Location {
    Point point;
    std::uint32_t byte;
};

bool operator==(const Location&, const Location&);
std::ostream& operator<<(std::ostream&, const Location&);

struct Range {
    Location start;
    Location end;
};

bool operator==(const Range&, const Range&);
std::ostream& operator<<(std::ostream&, const Range&);

struct Edit {
    Range range;
    std::string replacement;
};

bool operator==(const Edit&, const Edit&);
std::ostream& operator<<(std::ostream&, const Edit&);

// forward declarations
class Cursor;
class Tree;

/**
 * Wrapper for a 'TSNode'.
 *
 * Nodes can be named or anonymous (see [Named vs Anonymous
 * Nodes](https://tree-sitter.github.io/tree-sitter/using-parsers#named-vs-anonymous-nodes)). We are
 * mostly interested in named nodes.
 *
 * Nodes can be null (check with is_null).
 *
 * Note: This object is only valid for as long as the 'Tree' it was created from.
 */
class Node {
    TSNode node;
    const Tree& tree_;

public:
    explicit Node(TSNode node, const Tree& tree) noexcept;

    /**
     * Only for internal use.
     */
    TSNode raw() const;

    const Tree& tree() const;

    /**
     * Returns true if the node is null.
     */
    bool is_null() const;

    const char* type() const;

    /**
     * Get the nth child. This will also return anonymous nodes.
     */
    Node child(std::uint32_t index) const;
    std::uint32_t child_count() const;

    /**
     * Get the nth named child.
     */
    Node named_child(std::uint32_t index) const;
    std::uint32_t named_child_count() const;

    std::uint32_t start_byte() const;
    std::uint32_t end_byte() const;

    Point start_point() const;
    Point end_point() const;

    Location start() const;
    Location end() const;

    Range range() const;

    std::string text() const;

    std::string as_string() const;
};

/**
 * Parser for the Lua language.
 */
class Parser {
    std::unique_ptr<TSParser, void (*)(TSParser*)> parser;

public:
    Parser();

    // don't copy because we manage pointers
    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;

    // move constructor
    Parser(Parser&& other) noexcept;
    // move assignment
    Parser& operator=(Parser&& other) noexcept;
    friend void swap(Parser& self, Parser& other) noexcept;

    TSParser* raw();

    Tree parse_string(std::string&);
    // TODO other methods

private:
    Tree parse_string(const TSTree* old_tree, std::string&);
};

class Tree {
    std::unique_ptr<TSTree, void (*)(TSTree*)> tree;
    // maybe a separate Input type is better to be more flexible
    std::string source_;
    Parser& parser;

public:
    // explicit because this class handles the pointer as a ressource
    // automatic conversion from pointer to this type is dangerous
    explicit Tree(TSTree* tree, std::string& source, Parser& parser);

    // don't copy because we manage pointers
    Tree(const Tree&) = delete;
    Tree& operator=(const Tree&) = delete;

    // move constructor
    Tree(Tree&& other) noexcept;
    // move assignment
    Tree& operator=(Tree&& other) noexcept;
    friend void swap(Tree& self, Tree& other) noexcept;

    /**
     * Use with care. Mostly intended vor internal use in the wrapper types.
     *
     * WARNING: Never free or otherwise delete this pointer.
     */
    const TSTree* raw() const;

    const std::string& source() const;

    /**
     * The returned node is only valid as long as this tree is not destructed.
     */
    Node root_node() const;

    /**
     * Edit the syntax tree and source code.
     *
     * You need to call 'sync' after applying all the edits to bring the tree
     * back into a valid state.
     *
     * WARNING: Applying multiple edits is difficult if the replacement is a
     * different size than the original because the content after the edit will
     * move and subsequent edits will not have correct locations and this is
     * undefined behaviour.
     *
     * To avoid this you should apply the edits back to front. Meaning edits at
     * the end of the source should be applied before edit at the beginning.
     *
     * WARNING: Take care not to apply overlapping edits.
     */
    void edit(const Edit&);

    /**
     * Syncronizes the tree with the source code.
     *
     * You need to call this method after applying all the edits to bring the
     * tree back into a valid state.
     */
    void sync();

    void print_dot_graph(std::string path) const;
};

/**
 * Allows more efficient walking of a 'Tree' than with the methods on 'Node'.
 */
class Cursor {
    // TSTreeCursor internally allocates heap.
    // It can be copied with "ts_tree_cursor_copy" but it can not moved
    // because there is no easy way to clear the cursor. We can only reset the
    // cursor using a different tree.
    TSTreeCursor cursor;
    const Tree& tree;

public:
    explicit Cursor(Node) noexcept;
    explicit Cursor(const Tree&) noexcept;
    ~Cursor() noexcept;
    // copy constructor
    Cursor(const Cursor&) noexcept;
    // copy assignment
    Cursor& operator=(const Cursor&) noexcept;
    // delete move (see above)
    Cursor(Cursor&&) = delete;
    Cursor& operator=(Cursor&&) = delete;

    Node current_node() const;

    bool goto_parent();
    // TODO should these throw exceptions when there are no more named nodes?
    bool goto_first_child();
    bool goto_next_sibling();
};

} // namespace ts

#endif
