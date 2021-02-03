#include "ast.hpp"
#include "MiniLua/values.hpp"
#include <cassert>
#include <iostream>
#include <tree_sitter/tree_sitter.hpp>
#include <utility>

namespace minilua::details::ast {
// helper function
static auto convert_range(ts::Range range) -> Range {
    return Range{
        .start = {range.start.point.row, range.start.point.column, range.start.byte},
        .end = {range.end.point.row, range.end.point.column, range.end.byte},
    };
}
// Body
Body::Body(std::vector<ts::Node> node_vec) : nodes(std::move(node_vec)) {}
auto Body::return_statement() -> std::optional<Return> {
    if (nodes.empty() || nodes[nodes.size() - 1].type_id() != ts::NODE_RETURN_STATEMENT) {
        return std::nullopt;
    } else {
        return Return(nodes[nodes.size() - 1]);
    }
}
auto Body::statements() -> std::vector<Statement> {
    std::vector<ts::Node>::iterator end;
    std::vector<Statement> res;
    if (nodes.empty()) {
        return res;
    }
    if ((nodes.end() - 1)->type_id() == ts::NODE_RETURN_STATEMENT) {
        res.reserve(nodes.size() - 1);
        end = nodes.end() - 1;
    } else {
        res.reserve(nodes.size());
        end = nodes.end();
    }
    std::transform(
        nodes.begin(), end, std::back_inserter(res), [](ts::Node node) { return Statement(node); });
    return res;
}
// Identifier
Identifier::Identifier(ts::Node node) : id(node) {
    if (node.type_id() != ts::NODE_IDENTIFIER && node.type_id() != ts::NODE_METHOD &&
        node.type_id() != ts::NODE_PROPERTY_IDENTIFIER &&
        node.type_id() != ts::NODE_FUNCTION_NAME_FIELD) {
        throw std::runtime_error("not an identifier node" + to_string(node.type_id()));
    }
}
auto Identifier::string() const -> std::string { return this->id.text(); }
auto Identifier::range() const -> minilua::Range { return convert_range(this->id.range()); }
auto Identifier::raw() const -> ts::Node { return this->id; }
// Program
Program::Program(ts::Node node) : program(node) {
    if (node.type_id() != ts::NODE_PROGRAM) {
        throw std::runtime_error("not a program node");
    }
}
auto Program::body() const -> Body { return Body(this->program.named_children()); }
auto Program::range() const -> minilua::Range { return convert_range(this->program.range()); }
auto Program::raw() const -> ts::Node { return this->program; }
// BinaryOperation
BinaryOperation::BinaryOperation(ts::Node node) : bin_op(node) {
    if (node.type_id() != ts::NODE_BINARY_OPERATION) {
        throw std::runtime_error("not a binary_operation node");
    }
    assert(node.child_count() == 3);
}
auto BinaryOperation::left() const -> Expression {
    return Expression(this->bin_op.child(0).value());
}
auto BinaryOperation::right() const -> Expression {
    return Expression(this->bin_op.child(2).value());
}
auto BinaryOperation::binary_operator() const -> BinOpEnum {
    std::string op_str = bin_op.child(1)->text();
    if (op_str == "+"s) {
        return BinOpEnum::ADD;
    } else if (op_str == "-"s) {
        return BinOpEnum::SUB;
    } else if (op_str == "/"s) {
        return BinOpEnum::DIV;
    } else if (op_str == "*"s) {
        return BinOpEnum::MUL;
    } else if (op_str == "%"s) {
        return BinOpEnum::MOD;
    } else if (op_str == "^"s) {
        return BinOpEnum::POW;
    } else if (op_str == "<"s) {
        return BinOpEnum::LT;
    } else if (op_str == ">"s) {
        return BinOpEnum::GT;
    } else if (op_str == "<="s) {
        return BinOpEnum::LEQ;
    } else if (op_str == ">="s) {
        return BinOpEnum::GEQ;
    } else if (op_str == "==") {
        return BinOpEnum::EQ;
    } else if (op_str == "~="s) {
        return BinOpEnum::NEQ;
    } else if (op_str == ".."s) {
        return BinOpEnum::CONCAT;
    } else if (op_str == "and"s) {
        return BinOpEnum::AND;
    } else if (op_str == "or"s) {
        return BinOpEnum::OR;
    } else if (op_str == "<<"s) {
        return BinOpEnum::SHIFT_LEFT;
    } else if (op_str == ">>"s) {
        return BinOpEnum::SHIFT_RIGHT;
    } else if (op_str == "//"s) {
        return BinOpEnum::INT_DIV;
    } else if (op_str == "|"s) {
        return BinOpEnum::BIT_OR;
    } else if (op_str == "&"s) {
        return BinOpEnum::BIT_AND;
    } else if (op_str == "~"s) {
        return BinOpEnum::BIT_XOR;
    } else {
        throw std::runtime_error("Unknown Binary Operator: " + op_str);
    }
}
auto BinaryOperation::range() const -> minilua::Range {
    return convert_range(this->bin_op.range());
}
auto BinaryOperation::raw() const -> ts::Node { return this->bin_op; }
// UnaryOperation
UnaryOperation::UnaryOperation(ts::Node node) : un_op(node) {
    if (node.type_id() != ts::NODE_UNARY_OPERATION) {
        throw std::runtime_error("not an unary_operation node");
    }
    assert(node.child_count() == 2);
}

auto UnaryOperation::unary_operator() const -> UnOpEnum {
    if (this->un_op.child(0)->text() == "not"s) {
        return UnOpEnum::NOT;
    } else if (this->un_op.child(0)->text() == "-"s) {
        return UnOpEnum::NEG;
    } else if (this->un_op.child(0)->text() == "~"s) {
        return UnOpEnum::BWNOT;
    } else if (this->un_op.child(0)->text() == "#"s) {
        return UnOpEnum::LEN;
    } else {
        throw std::runtime_error("unknown Unary Operator: " + this->un_op.child(0)->text());
    }
}

auto UnaryOperation::expression() const -> Expression {
    return Expression(this->un_op.child(1).value());
}
auto UnaryOperation::range() const -> minilua::Range { return convert_range(this->un_op.range()); }
auto UnaryOperation::raw() const -> ts::Node { return this->un_op; }

/*std::ostream& operator<<(std::ostream& os, UnaryOperation& unaryOperation){
    return os << (int)unaryOperation.op() << unaryOperation.exp().type();
}*/

// ForStatement
ForStatement::ForStatement(ts::Node node) : for_statement(node) {
    if (node.type_id() != ts::NODE_FOR_STATEMENT) {
        throw std::runtime_error("not a for_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_LOOP_EXPRESSION);
}
auto ForStatement::body() const -> Body {
    std::vector<ts::Node> body = this->for_statement.named_children();
    body.erase(body.begin());
    return Body(body);
}

auto ForStatement::loop_expression() const -> LoopExpression {
    return LoopExpression(this->for_statement.named_child(0).value());
}
auto ForStatement::range() const -> minilua::Range {
    return convert_range(this->for_statement.range());
}
auto ForStatement::raw() const -> ts::Node { return this->for_statement; }

LoopExpression::LoopExpression(ts::Node node) : loop_exp(node) {
    if (node.type_id() != ts::NODE_LOOP_EXPRESSION) {
        throw std::runtime_error("not a loop_expression node");
    }
    assert(node.named_child_count() == 3 || node.named_child_count() == 4);
}
auto LoopExpression::variable() const -> Identifier {
    return Identifier(this->loop_exp.named_child(0).value());
}
auto LoopExpression::end() const -> Expression {
    if (this->loop_exp.named_child_count() == 4) {
        return Expression(this->loop_exp.named_child(3).value());
    } else {
        return Expression(this->loop_exp.named_child(2).value());
    }
}
auto LoopExpression::start() const -> Expression {
    return Expression(this->loop_exp.named_child(1).value());
}
auto LoopExpression::step() const -> std::optional<Expression> {
    if (this->loop_exp.named_child_count() == 4) {
        return Expression(this->loop_exp.named_child(2).value());
    } else {
        return std::nullopt;
    }
}
auto LoopExpression::range() const -> minilua::Range {
    return convert_range(this->loop_exp.range());
}
auto LoopExpression::raw() const -> ts::Node { return this->loop_exp; }

InLoopExpression::InLoopExpression(ts::Node node) : loop_exp(node) {
    if (node.type_id() != ts::NODE_LOOP_EXPRESSION) {
        throw std::runtime_error("not a in_loop_expression node");
    }
    assert(node.named_child_count() == 2);
}
auto InLoopExpression::loop_exps() const -> std::vector<Expression> {
    std::vector<ts::Node> children = this->loop_exp.named_child(1).value().named_children();
    std::vector<Expression> exps;
    exps.reserve(children.size());
    std::transform(children.begin(), children.end(), std::back_inserter(exps), [](ts::Node node) {
        return Expression(node);
    });
    return exps;
}
auto InLoopExpression::loop_vars() const -> std::vector<Identifier> {
    std::vector<ts::Node> children = this->loop_exp.named_child(0).value().named_children();
    std::vector<Identifier> loop_vars;
    loop_vars.reserve(children.size());
    std::transform(
        children.begin(), children.end(), std::back_inserter(loop_vars),
        [](ts::Node node) { return Identifier(node); });
    return loop_vars;
}
auto InLoopExpression::range() const -> minilua::Range {
    return convert_range(this->loop_exp.range());
}
auto InLoopExpression::raw() const -> ts::Node { return this->loop_exp; }

// ForInStatement
ForInStatement::ForInStatement(ts::Node node) : for_in(node) {
    if (node.type_id() != ts::NODE_FOR_IN_STATEMENT) {
        throw std::runtime_error("not a for_in_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0)->type_id() == ts::NODE_LOOP_EXPRESSION);
}
auto ForInStatement::loop_expression() const -> InLoopExpression {
    return InLoopExpression(this->for_in.named_child(0).value());
}
auto ForInStatement::body() const -> Body {
    std::vector<ts::Node> children = this->for_in.named_children();
    children.erase(children.begin());
    return Body(children);
}
auto ForInStatement::range() const -> minilua::Range { return convert_range(this->for_in.range()); }
auto ForInStatement::raw() const -> ts::Node { return this->for_in; }

// WhileStatement
WhileStatement::WhileStatement(ts::Node node) : while_statement(node) {
    if (node.type_id() != ts::NODE_WHILE_STATEMENT) {
        throw std::runtime_error("not a while_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}

auto WhileStatement::body() const -> Body {
    std::vector<ts::Node> body = this->while_statement.named_children();
    body.erase(body.begin());
    return Body(body);
}
auto WhileStatement::repeat_conditon() const -> Expression {
    return Expression(this->while_statement.named_child(0).value().named_child(0).value());
}
auto WhileStatement::range() const -> minilua::Range {
    return convert_range(this->while_statement.range());
}
auto WhileStatement::raw() const -> ts::Node { return this->while_statement; }

RepeatStatement::RepeatStatement(ts::Node node) : repeat_statement(node) {
    if (node.type_id() != ts::NODE_REPEAT_STATEMENT) {
        throw std::runtime_error("not a repeat_statement node");
    }
    assert(
        node.named_child_count() >= 1 &&
        (node.named_children().end() - 1)->type_id() == ts::NODE_CONDITION_EXPRESSION);
}
auto RepeatStatement::body() const -> Body {
    std::vector<ts::Node> body = this->repeat_statement.named_children();
    body.pop_back();
    return Body(body);
}
auto RepeatStatement::repeat_condition() const -> Expression {
    return Expression(
        this->repeat_statement.named_child(this->repeat_statement.named_child_count() - 1)
            .value()
            .named_child(0)
            .value());
}
auto RepeatStatement::range() const -> minilua::Range {
    return convert_range(this->repeat_statement.range());
}
auto RepeatStatement::raw() const -> ts::Node { return this->repeat_statement; }

// If
IfStatement::IfStatement(ts::Node node) : if_statement(node) {
    if (node.type_id() != ts::NODE_IF_STATEMENT) {
        throw std::runtime_error("not an if_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}
auto IfStatement::condition() const -> Expression {
    return Expression(this->if_statement.named_child(0).value().named_child(0).value());
}
auto IfStatement::else_statement() const -> std::optional<Else> {
    if (this->if_statement.named_child(this->if_statement.named_child_count() - 1).has_value() &&
        this->if_statement.named_child(this->if_statement.named_child_count() - 1)->type_id() ==
            ts::NODE_ELSE) {
        return Else(
            this->if_statement.named_child(this->if_statement.named_child_count() - 1).value());
    } else {
        return std::nullopt;
    }
}
auto IfStatement::elseifs() const -> std::vector<ElseIf> {
    if (this->if_statement.named_child_count() == 1) {
        return std::vector<ElseIf>();
    }
    std::vector<ts::Node> if_children = this->if_statement.named_children();
    std::vector<ts::Node>::iterator end;
    std::vector<ElseIf> res;
    if ((if_children.end() - 1)->type_id() == ts::NODE_ELSE) {
        end = if_children.end() - 1;
    } else {
        end = if_children.end();
    }
    auto begin = end;
    while ((begin - 1)->type_id() == ts::NODE_ELSEIF && begin - 1 != if_children.begin()) {
        begin--;
    }
    if (begin == end) {
        return std::vector<ElseIf>();
    }
    std::transform(begin, end, std::back_inserter(res), [](ts::Node node) { return ElseIf(node); });
    return res;
}
auto IfStatement::body() const -> Body {
    if (this->if_statement.named_child_count() == 1 ||
        (this->if_statement.named_child(1).has_value() &&
         this->if_statement.named_child(1)->type_id() == ts::NODE_ELSEIF)) {
        return Body(std::vector<ts::Node>());
    }
    std::vector<ts::Node> if_children = this->if_statement.named_children();
    std::vector<ts::Node>::iterator end;

    if ((if_children.end() - 1)->type_id() == ts::NODE_ELSE) {
        end = if_children.end() - 1;
    } else {
        end = if_children.end();
    }
    while ((end - 1)->type_id() == ts::NODE_ELSEIF) {
        end--;
    }
    return Body(std::vector<ts::Node>(if_children.begin() + 1, end));
}
auto IfStatement::range() const -> minilua::Range {
    return convert_range(this->if_statement.range());
}
auto IfStatement::raw() const -> ts::Node { return this->if_statement; }

// Else
Else::Else(ts::Node node) : else_statement(node) {
    if (node.type_id() != ts::NODE_ELSE) {
        throw std::runtime_error("Not an else_statement node");
    }
}
auto Else::body() const -> Body { return Body(else_statement.named_children()); }
auto Else::range() const -> minilua::Range { return convert_range(else_statement.range()); }
auto Else::raw() const -> ts::Node { return this->else_statement; }

// ElseIf
ElseIf::ElseIf(ts::Node node) : else_if(node) {
    if (node.type_id() != ts::NODE_ELSEIF) {
        throw std::runtime_error("not a else_if node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}
auto ElseIf::body() const -> Body {
    std::vector<ts::Node> body = this->else_if.named_children();
    body.erase(body.begin());
    return Body(body);
}
auto ElseIf::condition() const -> Expression {
    return Expression(this->else_if.named_child(0).value().named_child(0).value());
}
auto ElseIf::range() const -> minilua::Range { return convert_range(this->else_if.range()); }
auto ElseIf::raw() const -> ts::Node { return this->else_if; }

// Return
Return::Return(ts::Node node) : expressions(node) {}
auto Return::exp_list() const -> std::vector<Expression> {
    std::vector<ts::Node> exps = this->expressions.named_children();
    std::vector<ts::Node>::iterator end;
    std::vector<Expression> res;
    end = exps.end();
    res.reserve(exps.size());
    std::transform(
        exps.begin(), end, std::back_inserter(res), [](ts::Node node) { return Expression(node); });
    return res;
}
auto Return::range() const -> minilua::Range { return convert_range(this->expressions.range()); }
auto Return::raw() const -> ts::Node { return this->expressions; }

// VariableDeclaration
VariableDeclaration::VariableDeclaration(ts::Node node) : var_dec(node) {
    if (node.type_id() != ts::NODE_VARIABLE_DECLARATION &&
        node.type_id() != ts::NODE_LOCAL_VARIABLE_DECLARATION) {
        throw std::runtime_error("not a variable_declaration node");
    } else {
        local_dec = node.type_id() == ts::NODE_LOCAL_VARIABLE_DECLARATION;
    }
}
auto VariableDeclaration::declarations() const -> std::vector<Expression> {
    std::vector<Expression> res;
    if (this->local_dec) {
        std::vector<ts::Node> nodes = this->var_dec.named_children();
        res.reserve(nodes.size() - 1);
        std::transform(nodes.begin() + 1, nodes.end(), std::back_inserter(res), [](ts::Node node) {
            return Expression(node);
        });
        return res;
    } else {
        std::vector<ts::Node> nodes = this->var_dec.named_children();
        auto it = nodes.begin();
        while (it->type_id() == ts::NODE_VARIABLE_DECLARATOR) {
            it++;
        }
        res.reserve(nodes.size() - (it - nodes.begin()));
        std::transform(it, nodes.end(), std::back_inserter(res), [](ts::Node node) {
            return Expression(node);
        });
        return res;
    }
}
auto VariableDeclaration::declarators() const -> std::vector<VariableDeclarator> {
    if (this->local_dec) {
        std::vector<ts::Node> nodes = this->var_dec.named_child(0).value().named_children();
        std::vector<VariableDeclarator> res;
        res.reserve(nodes.size());
        std::transform(nodes.begin(), nodes.end(), std::back_inserter(res), [](ts::Node node) {
            return VariableDeclarator(node);
        });
        return res;
    } else {
        std::vector<ts::Node> nodes = this->var_dec.named_children();
        std::vector<VariableDeclarator> res;
        auto it = nodes.begin();
        while (it->type_id() == ts::NODE_VARIABLE_DECLARATOR) {
            it++;
        }
        res.reserve(it - nodes.begin());
        std::transform(nodes.begin(), it, std::back_inserter(res), [](ts::Node node) {
            return VariableDeclarator(node);
        });
        return res;
    }
}
auto VariableDeclaration::local() const -> bool { return this->local_dec; }
auto VariableDeclaration::range() const -> minilua::Range {
    return convert_range(this->var_dec.range());
}
auto VariableDeclaration::raw() const -> ts::Node { return this->var_dec; }

// VariableDeclarator
VariableDeclarator::VariableDeclarator(ts::Node node) : dec(node) {
    if (node.type_id() != ts::NODE_VARIABLE_DECLARATOR && node.type_id() != ts::NODE_IDENTIFIER &&
        node.type_id() != ts::NODE_FIELD_EXPRESSION && node.type_id() != ts::NODE_TABLE_INDEX) {
        throw std::runtime_error("not a variable declarator");
    }
}
auto VariableDeclarator::options() const -> std::variant<Identifier, FieldExpression, TableIndex> {
    if (this->dec.type_id() == ts::NODE_IDENTIFIER) {
        return Identifier(this->dec);
    } else if (this->dec.type_id() == ts::NODE_FIELD_EXPRESSION) {
        return FieldExpression(this->dec);
    } else if (this->dec.type_id() == ts::NODE_TABLE_INDEX) {
        return TableIndex(this->dec);
    }
    if (this->dec.named_child(0).has_value()) {
        if (this->dec.named_child(0)->type_id() == ts::NODE_IDENTIFIER) {
            return Identifier(this->dec.named_child(0).value());
        } else if (this->dec.named_child(0)->type_id() == ts::NODE_FIELD_EXPRESSION) {
            return FieldExpression(this->dec.named_child(0).value());
        } else if (this->dec.named_child(0)->type_id() == ts::NODE_TABLE_INDEX) {
            return TableIndex(this->dec.named_child(0).value());
        } else {
            throw std::runtime_error("invalid variable declarator");
        }
    } else {
        throw std::runtime_error("invalid variable declarator");
    }
}
auto VariableDeclarator::range() const -> minilua::Range {
    return convert_range(this->dec.range());
}
auto VariableDeclarator::raw() const -> ts::Node { return this->dec; }

// TableIndex
TableIndex::TableIndex(ts::Node node) : table_index(node) {
    if (node.type_id() != ts::NODE_TABLE_INDEX) {
        throw std::runtime_error("not a table_index node");
    }
    assert(node.named_child_count() == 2);
}
auto TableIndex::table() const -> Prefix {
    return Prefix(this->table_index.named_child(0).value());
}
auto TableIndex::index() const -> Expression {
    return Expression(this->table_index.named_child(1).value());
}
auto TableIndex::range() const -> minilua::Range {
    return convert_range(this->table_index.range());
}
auto TableIndex::raw() const -> ts::Node { return this->table_index; }

// DoStatement
DoStatement::DoStatement(ts::Node node) : do_statement(node) {
    if (node.type_id() != ts::NODE_DO_STATEMENT) {
        throw std::runtime_error("not a do_statement node");
    }
}
auto DoStatement::body() const -> Body { return Body(this->do_statement.named_children()); }
auto DoStatement::range() const -> minilua::Range {
    return convert_range(this->do_statement.range());
}
auto DoStatement::raw() const -> ts::Node { return this->do_statement; }

// FieldExpression
FieldExpression::FieldExpression(ts::Node node) : exp(node) {
    if (node.type_id() != ts::NODE_FIELD_EXPRESSION) {
        throw std::runtime_error("not a field_expression node");
    }
    assert(node.named_child_count() == 2);
}
auto FieldExpression::table_id() const -> Prefix {
    return Prefix(this->exp.named_child(0).value());
}
auto FieldExpression::property_id() const -> Identifier {
    return Identifier(this->exp.named_child(1).value());
}
auto FieldExpression::range() const -> minilua::Range { return convert_range(this->exp.range()); }
auto FieldExpression::raw() const -> ts::Node { return this->exp; }

// Label
Label::Label(ts::Node node) : label(node) {
    if (node.type_id() != ts::NODE_LABEL_STATEMENT) {
        throw std::runtime_error("not a label node");
    }
    assert(node.named_child_count() == 1);
}
auto Label::id() const -> Identifier { return Identifier(this->label.named_child(0).value()); }
auto Label::range() const -> minilua::Range { return convert_range(this->label.range()); }
auto Label::raw() const -> ts::Node { return this->label; }

// GoTo
GoTo::GoTo(ts::Node node) : go_to(node) {
    if (node.type_id() != ts::NODE_GOTO_STATEMENT) {
        throw std::runtime_error("not a go_to node");
    }
    assert(node.named_child_count() == 1);
}
auto GoTo::label() const -> Identifier { return Identifier(this->go_to.named_child(0).value()); }
auto GoTo::range() const -> minilua::Range { return convert_range(this->go_to.range()); }
auto GoTo::raw() const -> ts::Node { return this->go_to; }

// Parameters
Parameters::Parameters(ts::Node node) : parameters(node) {
    if (node.type_id() != ts::NODE_PARAMETERS) {
        throw std::runtime_error("not a parameters node");
    }
}
auto Parameters::leading_self() const -> bool {
    if (this->parameters.named_child_count() < 1) {
        return false;
    } else {
        return this->parameters.named_child(0)->type_id() == ts::NODE_SELF;
    }
}
auto Parameters::spread() const -> SpreadPos {
    if (this->parameters.named_child_count() < 1) {
        return SpreadPos::NO_SPREAD;
    } else if (this->parameters.named_child(0)->type_id() == ts::NODE_SPREAD) {
        return SpreadPos::BEGIN;
    } else if (
        this->parameters.named_child(this->parameters.named_child_count() - 1)->type_id() ==
        ts::NODE_SPREAD) {
        return SpreadPos::END;
    } else {
        return SpreadPos::NO_SPREAD;
    }
}
auto Parameters::params() const -> std::vector<Identifier> {
    SpreadPos sp = spread();
    std::vector<Identifier> res;
    std::vector<ts::Node> children = this->parameters.named_children();
    if (leading_self()) {
        if (sp == NO_SPREAD) {
            if (this->parameters.named_child_count() < 1) {
                return res;
            }
            res.reserve(this->parameters.named_child_count() - 1);
            std::transform(
                children.begin() + 1, children.end(), std::back_inserter(res),
                [](ts::Node node) { return Identifier(node); });
        } else {
            if (this->parameters.named_child_count() < 2) {
                return res;
            }
            res.reserve(this->parameters.named_child_count() - 2);
            std::transform(
                children.begin() + 1, children.end() - 1, std::back_inserter(res),
                [](ts::Node node) { return Identifier(node); });
        }
    } else {
        switch (sp) {
        case NO_SPREAD:
            res.reserve(this->parameters.named_child_count());
            std::transform(
                children.begin(), children.end(), std::back_inserter(res),
                [](ts::Node node) { return Identifier(node); });
            break;
        case BEGIN:
            if (this->parameters.named_child_count() < 1) {
                return res;
            }
            res.reserve(this->parameters.named_child_count() - 1);
            std::transform(
                children.begin() + 1, children.end(), std::back_inserter(res),
                [](ts::Node node) { return Identifier(node); });
            break;
        case END:
            if (this->parameters.named_child_count() < 1) {
                return res;
            }
            res.reserve(this->parameters.named_child_count() - 1);
            std::transform(
                children.begin(), children.end() - 1, std::back_inserter(res),
                [](ts::Node node) { return Identifier(node); });
            break;
        }
    }
    return res;
}
auto Parameters::range() const -> minilua::Range { return convert_range(this->parameters.range()); }
auto Parameters::raw() const -> ts::Node { return this->parameters; }

// FunctionName
FunctionName::FunctionName(ts::Node node) : func_name(node) {
    if (node.type_id() != ts::NODE_FUNCTION_NAME && node.type_id() != ts::NODE_IDENTIFIER) {
        throw std::runtime_error("Not a function_name node");
    }
}
auto FunctionName::method() const -> std::optional<Identifier> {
    if (this->func_name.type_id() == ts::NODE_IDENTIFIER) {
        return std::nullopt;
    }
    if (this->func_name.named_child(this->func_name.named_child_count() - 1)->type_id() ==
        ts::NODE_METHOD) {
        return Identifier(
            this->func_name.named_child(this->func_name.named_child_count() - 1).value());
    } else {
        return std::nullopt;
    }
}
auto FunctionName::identifier() const -> std::vector<Identifier> {
    if (this->func_name.type_id() == ts::NODE_IDENTIFIER) {
        return std::vector<Identifier>{Identifier(this->func_name)};
    }
    if (this->func_name.named_child(0)->type_id() == ts::NODE_IDENTIFIER) {
        return std::vector<Identifier>{Identifier(this->func_name.named_child(0).value())};
    } else {
        std::vector<Identifier> res;
        std::vector<ts::Node> children = this->func_name.named_child(0).value().named_children();
        res.reserve(children.size());
        std::transform(
            children.begin(), children.end(), std::back_inserter(res),
            [](ts::Node node) { return Identifier(node); });
        return res;
    }
}
auto FunctionName::range() const -> minilua::Range {
    return convert_range(this->func_name.range());
}
auto FunctionName::raw() const -> ts::Node { return this->func_name; }

// FunctionDefinition
FunctionDefinition::FunctionDefinition(ts::Node node) : func_def(node) {
    if (node.type_id() != ts::NODE_FUNCTION_DEFINITION) {
        throw std::runtime_error("not a function_definition node");
    }
}
auto FunctionDefinition::parameters() const -> Parameters {
    return Parameters(this->func_def.named_child(0).value());
}
auto FunctionDefinition::body() const -> Body {
    std::vector<ts::Node> children = this->func_def.named_children();
    return Body(std::vector<ts::Node>(children.begin() + 1, children.end()));
}
auto FunctionDefinition::range() const -> minilua::Range {
    return convert_range(this->func_def.range());
}
auto FunctionDefinition::raw() const -> ts::Node { return this->func_def; }

// FunctionStatement
FunctionStatement::FunctionStatement(ts::Node node) : func_stat(node) {
    if (node.type_id() != ts::NODE_FUNCTION && node.type_id() != ts::NODE_LOCAL_FUNCTION) {
        throw std::runtime_error("not a function(_statement) node");
    }
    is_local = node.type_id() == ts::NODE_LOCAL_FUNCTION;
}
auto FunctionStatement::body() const -> Body {
    std::vector<ts::Node> children = this->func_stat.named_children();
    return Body(std::vector<ts::Node>(children.begin() + 2, children.end()));
}
auto FunctionStatement::name() const -> FunctionName {
    return FunctionName(this->func_stat.named_child(0).value());
}
auto FunctionStatement::parameters() const -> Parameters {
    return Parameters(this->func_stat.named_child(1).value());
}
auto FunctionStatement::local() const -> bool { return this->is_local; }
auto FunctionStatement::range() const -> minilua::Range {
    return convert_range(this->func_stat.range());
}
auto FunctionStatement::raw() const -> ts::Node { return this->func_stat; }

// FunctionCall
FunctionCall::FunctionCall(ts::Node node) : func_call(node) {
    if (node.type_id() != ts::NODE_FUNCTION_CALL) {
        throw runtime_error("not a function_call node");
    }
    assert(node.named_child_count() == 2 || node.named_child_count() == 3);
}
auto FunctionCall::method() const -> std::optional<Identifier> {
    if (this->func_call.named_child_count() == 3) {
        return Identifier(this->func_call.named_child(1).value());
    } else {
        return std::nullopt;
    }
}
auto FunctionCall::id() const -> Prefix { return Prefix(this->func_call.named_child(0).value()); }
auto FunctionCall::args() const -> std::vector<Expression> {
    std::vector<ts::Node> arg_nodes =
        this->func_call.named_child(this->func_call.named_child_count() - 1)
            .value()
            .named_children();
    std::vector<Expression> args;
    args.reserve(arg_nodes.size());
    std::transform(arg_nodes.begin(), arg_nodes.end(), std::back_inserter(args), [](ts::Node node) {
        return Expression(node);
    });
    return args;
}
auto FunctionCall::range() const -> minilua::Range {
    return convert_range(this->func_call.range());
}
auto FunctionCall::raw() const -> ts::Node { return this->func_call; }

// Table
Table::Table(ts::Node node) : table(node) {
    if (node.type_id() != ts::NODE_TABLE) {
        throw runtime_error("not a table node");
    }
}
auto Table::fields() const -> std::vector<Field> {
    if (this->table.named_child_count() < 1) {
        return std::vector<Field>();
    }
    std::vector<Field> fields;
    fields.reserve(this->table.named_child_count());
    std::vector<ts::Node> nodes = this->table.named_children();
    std::transform(nodes.begin(), nodes.end(), std::back_inserter(fields), [](ts::Node node) {
        return Field(node);
    });
    return fields;
}
auto Table::range() const -> minilua::Range { return convert_range(this->table.range()); }
auto Table::raw() const -> ts::Node { return this->table; }

// Field
Field::Field(ts::Node node) : field(node) {
    if (node.type_id() != ts::NODE_FIELD) {
        throw runtime_error("not a field node");
    }
    assert(node.named_child_count() == 1 || node.named_child_count() == 2);
}
auto Field::content() const -> std::variant<
    std::pair<Expression, Expression>, std::pair<Identifier, Expression>, Expression> {
    if (this->field.named_child_count() < 2) {
        return Expression(this->field.named_child(0).value());
    } else if (this->field.child(0)->text() == "[") {
        return std::pair<Expression, Expression>(
            Expression(this->field.named_child(0).value()),
            Expression(field.named_child(1).value()));
    } else {
        return std::pair<Identifier, Expression>(
            Identifier(this->field.named_child(0).value()),
            Expression(field.named_child(1).value()));
    }
}
auto Field::range() const -> minilua::Range { return convert_range(this->field.range()); }
auto Field::raw() const -> ts::Node { return this->field; }

// Prefix
Prefix::Prefix(ts::Node node) : prefix(node) {
    if (!(node.type_id() == ts::NODE_SELF || node.type_id() == ts::NODE_FUNCTION_CALL ||
          node.type_id() == ts::NODE_IDENTIFIER || node.type_id() == ts::NODE_FIELD_EXPRESSION ||
          node.type_id() == ts::NODE_TABLE_INDEX)) {
        throw std::runtime_error("Not a prefix-node");
    }
}
auto Prefix::options() const -> std::variant<Self, VariableDeclarator, FunctionCall, Expression> {
    if (this->prefix.type_id() == ts::NODE_SELF) {
        return Self();
    } else if (this->prefix.type_id() == ts::NODE_FUNCTION_CALL) {
        return FunctionCall(this->prefix);
    } else if (this->prefix.child_count() > 0 && this->prefix.child(0)->text() == "(") {
        return Expression(this->prefix.child(1).value());
    } else if (
        this->prefix.type_id() == ts::NODE_IDENTIFIER ||
        this->prefix.type_id() == ts::NODE_FIELD_EXPRESSION ||
        this->prefix.type_id() == ts::NODE_TABLE_INDEX) {
        return VariableDeclarator(this->prefix);
    } else {
        throw std::runtime_error("Not a prefix-node");
    }
}
auto Prefix::range() const -> minilua::Range { return convert_range(this->prefix.range()); }
auto Prefix::raw() const -> ts::Node { return this->prefix; }

// Expression
Expression::Expression(ts::Node node) : exp(node) {
    if (!(node.type_id() == ts::NODE_SPREAD || node.type_id() == ts::NODE_FUNCTION_DEFINITION ||
          node.type_id() == ts::NODE_TABLE || node.type_id() == ts::NODE_BINARY_OPERATION ||
          node.type_id() == ts::NODE_UNARY_OPERATION || node.type_id() == ts::NODE_STRING ||
          node.type_id() == ts::NODE_NUMBER || node.type_id() == ts::NODE_NIL ||
          node.type_id() == ts::NODE_FALSE || node.type_id() == ts::NODE_TRUE ||
          node.type_id() == ts::NODE_IDENTIFIER || node.type_id() == ts::NODE_SELF ||
          node.type_id() == ts::NODE_FUNCTION_CALL || node.type_id() == ts::NODE_FIELD_EXPRESSION ||
          node.type_id() == ts::NODE_TABLE_INDEX || node.child(0)->text() == "(")) {
        throw std::runtime_error("Not an expression-node");
    }
}
auto Expression::options() const -> std::variant<
    Spread, Prefix, FunctionDefinition, Table, BinaryOperation, UnaryOperation, Literal,
    Identifier> {
    if (this->exp.type_id() == ts::NODE_SPREAD) {
        return Spread();
    } else if (this->exp.type_id() == ts::NODE_FUNCTION_DEFINITION) {
        return FunctionDefinition(this->exp);
    } else if (this->exp.type_id() == ts::NODE_TABLE) {
        return Table(this->exp);
    } else if (this->exp.type_id() == ts::NODE_BINARY_OPERATION) {
        return BinaryOperation(this->exp);
    } else if (this->exp.type_id() == ts::NODE_UNARY_OPERATION) {
        return UnaryOperation(this->exp);
    } else if (this->exp.type_id() == ts::NODE_STRING) {
        return Literal(LiteralType::STRING, this->exp.text(), this->exp.range());
    } else if (this->exp.type_id() == ts::NODE_NUMBER) {
        return Literal(LiteralType::NUMBER, this->exp.text(), this->exp.range());
    } else if (this->exp.type_id() == ts::NODE_NIL) {
        return Literal(LiteralType::NIL, this->exp.text(), this->exp.range());
    } else if (this->exp.type_id() == ts::NODE_TRUE) {
        return Literal(LiteralType::TRUE, this->exp.text(), this->exp.range());
    } else if (this->exp.type_id() == ts::NODE_FALSE) {
        return Literal(LiteralType::FALSE, this->exp.text(), this->exp.range());
    } else if (this->exp.type_id() == ts::NODE_IDENTIFIER) {
        return Identifier(this->exp);
    } else if (
        this->exp.type_id() == ts::NODE_SELF || this->exp.type_id() == ts::NODE_FUNCTION_CALL ||
        this->exp.type_id() == ts::NODE_FIELD_EXPRESSION ||
        this->exp.type_id() == ts::NODE_TABLE_INDEX ||
        (this->exp.child(0).has_value() && this->exp.child(0)->text() == "(")) {
        return Prefix(this->exp);
    } else {
        throw std::runtime_error("Not an expression-node");
    }
}
auto Expression::range() const -> minilua::Range { return convert_range(this->exp.range()); }
auto Expression::raw() const -> ts::Node { return this->exp; }

// Statement
Statement::Statement(ts::Node node) : statement(node) {
    if (!(node.type_id() == ts::NODE_EXPRESSION ||
          node.type_id() == ts::NODE_VARIABLE_DECLARATION ||
          node.type_id() == ts::NODE_LOCAL_VARIABLE_DECLARATION ||
          node.type_id() == ts::NODE_DO_STATEMENT || node.type_id() == ts::NODE_IF_STATEMENT ||
          node.type_id() == ts::NODE_WHILE_STATEMENT ||
          node.type_id() == ts::NODE_REPEAT_STATEMENT || node.type_id() == ts::NODE_FOR_STATEMENT ||
          node.type_id() == ts::NODE_FOR_IN_STATEMENT ||
          node.type_id() == ts::NODE_GOTO_STATEMENT || node.type_id() == ts::NODE_BREAK_STATEMENT ||
          node.type_id() == ts::NODE_LABEL_STATEMENT || node.type_id() == ts::NODE_FUNCTION ||
          node.type_id() == ts::NODE_LOCAL_FUNCTION || node.type_id() == ts::NODE_FUNCTION_CALL ||
          (node.child(0).has_value() && node.child(0)->text() == ";"))) {
        throw std::runtime_error("Not a statement-node " + std::string(node.type()));
    }
}
auto Statement::options() const -> std::variant<
    VariableDeclaration, DoStatement, IfStatement, WhileStatement, RepeatStatement, ForStatement,
    ForInStatement, GoTo, Break, Label, FunctionStatement, FunctionCall, Expression> {
    if (this->statement.type_id() == ts::NODE_EXPRESSION) {
        return Expression(this->statement.named_child(0).value());
    } else if (
        this->statement.type_id() == ts::NODE_VARIABLE_DECLARATION ||
        this->statement.type_id() == ts::NODE_LOCAL_VARIABLE_DECLARATION) {
        return VariableDeclaration(this->statement);
    } else if (this->statement.type_id() == ts::NODE_DO_STATEMENT) {
        return DoStatement(this->statement);
    } else if (this->statement.type_id() == ts::NODE_IF_STATEMENT) {
        return IfStatement(this->statement);
    } else if (this->statement.type_id() == ts::NODE_WHILE_STATEMENT) {
        return WhileStatement(this->statement);
    } else if (this->statement.type_id() == ts::NODE_REPEAT_STATEMENT) {
        return RepeatStatement(this->statement);
    } else if (this->statement.type_id() == ts::NODE_FOR_STATEMENT) {
        return ForStatement(this->statement);
    } else if (this->statement.type_id() == ts::NODE_FOR_IN_STATEMENT) {
        return ForInStatement(this->statement);
    } else if (this->statement.type_id() == ts::NODE_GOTO_STATEMENT) {
        return GoTo(statement);
    } else if (this->statement.type_id() == ts::NODE_BREAK_STATEMENT) {
        return Break();
    } else if (this->statement.type_id() == ts::NODE_LABEL_STATEMENT) {
        return Label(this->statement);
    } else if (this->statement.type_id() == ts::NODE_FUNCTION) {
        return FunctionStatement(this->statement);
    } else if (this->statement.type_id() == ts::NODE_LOCAL_FUNCTION) {
        return FunctionStatement(this->statement);
    } else if (this->statement.type_id() == ts::NODE_FUNCTION_CALL) {
        return FunctionCall(this->statement);
    } else {
        throw std::runtime_error("Not a statement-node");
    }
}
auto Statement::range() const -> minilua::Range { return convert_range(this->statement.range()); }
auto Statement::raw() const -> ts::Node { return this->statement; }

Literal::Literal(LiteralType type, std::string string, ts::Range range)
    : literal_content(string), literal_type(type), literal_range(range){};
auto Literal::type() const -> LiteralType { return this->literal_type; }
auto Literal::content() const -> std::string { return this->literal_content; }
auto Literal::range() const -> minilua::Range { return convert_range(this->literal_range); }
} // namespace minilua::details::ast