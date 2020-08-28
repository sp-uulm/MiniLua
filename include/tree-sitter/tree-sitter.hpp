#ifndef TREE_SITTER_HPP
#define TREE_SITTER_HPP

#include <memory>
#include <stdexcept>
#include <tree_sitter/api.h>
#include <utility>

extern "C" const TSLanguage* tree_sitter_lua();

/// Wrapper types and helper functions for Tree-Sitter.
namespace ts {

class Tree {
    std::unique_ptr<TSTree, void(*)(TSTree*)> tree;

public:
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

    TSNode get_root_node() const noexcept;
};

class Parser {
    std::unique_ptr<TSParser, void(*)(TSParser*)> parser;

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
