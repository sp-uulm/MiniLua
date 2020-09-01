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
 * Numeric representation of the type of a node.
 */
typedef TSSymbol TypeId;

/**
 * Contains a source location.
 */
struct Point {
    std::uint32_t row;
    std::uint32_t column;
};

bool operator==(const Point&, const Point&);
bool operator!=(const Point&, const Point&);
std::ostream& operator<<(std::ostream&, const Point&);

struct Location {
    Point point;
    std::uint32_t byte;
};

bool operator==(const Location&, const Location&);
bool operator!=(const Location&, const Location&);
std::ostream& operator<<(std::ostream&, const Location&);

struct Range {
    Location start;
    Location end;
};

bool operator==(const Range&, const Range&);
bool operator!=(const Range&, const Range&);
std::ostream& operator<<(std::ostream&, const Range&);

struct Edit {
    Range range;
    std::string replacement;
};

bool operator==(const Edit&, const Edit&);
bool operator!=(const Edit&, const Edit&);
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
 * If the tree was edited methods on the node might return wrong results. In this
 * case you should retrieve the node from the tree again.
 */
class Node {
    TSNode node;
    const Tree& tree_;

public:
    /**
     * Creates a new node from the given tree-sitter node and the tree.
     *
     * NOTE: Should only be used internally.
     */
    explicit Node(TSNode node, const Tree& tree) noexcept;

    /**
     * Only for internal use.
     */
    TSNode raw() const;

    /**
     * Get the tree this node was created from.
     */
    const Tree& tree() const;

    /**
     * Get the string representation of the type of the node.
     */
    const char* type() const;

    /**
     * Get the numeric representation of the type of the node.
     *
     * In tree-sitter this is called *symbol*.
     *
     * NOTE: There is currently no real way to use it because there are no
     * constants available to compare to.
     */
    TypeId type_id() const;

    /**
     * Check if the node is null.
     *
     * Methods like 'child' or 'next_sibling' can return null nodes.
     */
    bool is_null() const;

    /**
     * Check if the node is named.
     */
    bool is_named() const;

    /**
     * Check if the node is *missing*.
     *
     * Missing nodes are used to recover from some kinds of syntax errors.
     */
    bool is_missing() const;

    /**
     * Check if the node is *extra*.
     *
     * Extra nodes represent things like comments.
     */
    bool is_extra() const;

    /**
     * Check if the node has been edited.
     */
    bool has_changes() const;

    /**
     * Get true if the node is a syntax error or contains any syntax errors.
     */
    bool has_error() const;

    /**
     * Gets the nodes parent.
     *
     * Can return a null node.
     */
    Node parent() const;

    /**
     * Get the n-th child (0 indexed).
     *
     * This will also return anonymous nodes.
     *
     * Can return a null node.
     */
    Node child(std::uint32_t index) const;

    /**
     * Get the count of all children.
     */
    std::uint32_t child_count() const;

    /**
     * Get the n-th named child (0 indexed).
     *
     * This will not return anonymous nodes and the index only considers named
     * nodes.
     *
     * Can return a null node.
     */
    Node named_child(std::uint32_t index) const;

    /**
     * Get the count of named children.
     */
    std::uint32_t named_child_count() const;

    /**
     * Get the node's next sibling.
     *
     * This will also return anonymous nodes.
     *
     * Can return a null node.
     */
    Node next_sibling() const;

    /**
     * Get the node's previous sibling.
     *
     * This will also return anonymous nodes.
     *
     * Can return a null node.
     */
    Node prev_sibling() const;

    /**
     * Get the node's next *named* sibling.
     *
     * This will not return anonymous nodes.
     *
     * Can return a null node.
     */
    Node next_named_sibling() const;

    /**
     * Get the node's previous *named* sibling.
     *
     * This will not return anonymous nodes.
     *
     * Can return a null node.
     */
    Node prev_named_sibling() const;

    /**
     * Get the start position as a byte offset.
     */
    std::uint32_t start_byte() const;

    /**
     * Get the end position as a byte offset.
     *
     * Returns the position after the last character.
     */
    std::uint32_t end_byte() const;

    /**
     * Get the start position as a 'Point' (row + column).
     */
    Point start_point() const;

    /**
     * Get the end position as a 'Point' (row + column).
     */
    Point end_point() const;

    /**
     * Get the start position as a 'Location' ('Point' + byte).
     */
    Location start() const;

    /**
     * Get the end position as a 'Location' ('Point' + byte).
     */
    Location end() const;

    /**
     * Get the 'Range' of the node (start and end location).
     */
    Range range() const;

    /**
     * Get the original string this node represents.
     */
    std::string text() const;

    /**
     * Returns the syntax tree starting from node represented as an s-expression.
     */
    std::string as_s_expr() const;
};

bool operator==(const Node& lhs, const Node& rhs);
bool operator!=(const Node& lhs, const Node& rhs);

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

    // use pointer instead of reference because we need to reassign in the copy
    // constructor
    Parser* parser;

public:
    // explicit because this class handles the pointer as a ressource
    // automatic conversion from pointer to this type is dangerous
    explicit Tree(TSTree* tree, const std::string& source, Parser& parser);

    // TSTree* can be safely (and fast) copied using ts_tree_copy
    Tree(const Tree&);
    Tree& operator=(const Tree&);

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
