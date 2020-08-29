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

public:
    Node(TSNode node) noexcept;

    /**
     * Returns true if the node is null.
     */
    bool is_null() const noexcept;

    const char* get_type() const;

    Node get_child(std::uint32_t index) const;
    Node get_named_child(std::uint32_t index) const;
    std::uint32_t get_named_child_count() const;

    std::string as_string() const;
};

class Tree {
    std::unique_ptr<TSTree, void (*)(TSTree*)> tree;

public:
    // explicit because this class handles the pointer as a ressource
    // automatic conversion from pointer to this type is dangerous
    explicit Tree(TSTree* tree);

    // don't copy because we manage pointers
    Tree(const Tree&) = delete;
    Tree& operator=(const Tree&) = delete;

    // move constructor
    Tree(Tree&& other) noexcept;
    // move assignment
    Tree& operator=(Tree&& other) noexcept;
    friend void swap(Tree& self, Tree& other) noexcept;

    const TSTree* get_raw() const noexcept;

    /**
     * The returned node is only valid as long as this tree is not destructed.
     */
    Node get_root_node() const noexcept;
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

    // will reulse parts of old_tree
    // source changes must already be exctly present in the old_tree (see ts_tree_edit)
    Tree parse_string(Tree old_tree, std::string&);
    Tree parse_string(std::string&);
    // TODO other methods

private:
    Tree parse_string(const TSTree* old_tree, std::string&);
};

} // namespace ts

#endif
