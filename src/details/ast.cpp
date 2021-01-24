#include "ast.hpp"
#include <assert.h>
#include <iostream>
#include <tree_sitter/tree_sitter.hpp>
#include <utility>

namespace minilua::details::ast {

// Body
Body::Body(std::vector<ts::Node> node_vec) : nodes(std::move(node_vec)) {}
auto Body::ret() -> std::optional<Return> {
    if (nodes.empty() || nodes[nodes.size() - 1].type_id() != ts::NODE_RETURN_STATEMENT) {
        return {};
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
auto Identifier::str() -> std::string { return id.text(); }
auto Identifier::raw() -> ts::Node { return this->id; }
// Program
Program::Program(ts::Node node) : program(node) {
    if (node.type_id() != ts::NODE_PROGRAM) {
        throw std::runtime_error("not a program node");
    }
}
auto Program::body() -> Body { return Body(program.named_children()); }
auto Program::raw() -> ts::Node { return this->program; }
// BinOpEnum
/*static std::ostream& operator<<(std::ostream& os, BinOpEnum& binOpEnum) {
    switch (binOpEnum) {
    case BinOpEnum::ADD:
        return os << '+';
    case BinOpEnum::SUB:
        return os << '-';
    case BinOpEnum::MUL:
        return os << '*';
    case BinOpEnum::DIV:
        return os << '/';
    case BinOpEnum::POW:
        return os << '^';
    case BinOpEnum::MOD:
        return os << '%';
    case BinOpEnum::LEQ:
        return os << "<=";
    case BinOpEnum::GEQ:
        return os << ">=";
    case BinOpEnum::EQ:
        return os << "==";
    case BinOpEnum::LT:
        return os << '<';
    case BinOpEnum::GT:
        return os << '>';
    case BinOpEnum::NEQ:
        return os << "~=";
    case BinOpEnum::AND:
        return os << "and";
    case BinOpEnum::OR:
        return os << "or";
    case BinOpEnum::CONCAT:
        return os << "..";
    case BinOpEnum::BSL:
        return os << "<<";
    case BinOpEnum::BSR:
        return os << ">>";
    case BinOpEnum::BWNOT:
        return os << "~";
    case BinOpEnum::BWAND:
        return os << "&";
    case BinOpEnum::BWOR:
        return os << "|";
    case BinOpEnum::INTDIV:
        return os << "//";
    }
    throw std::runtime_error("invalid binary operator");
};*/
// BinaryOperation
BinaryOperation::BinaryOperation(ts::Node node) : bin_op(node) {
    if (node.type_id() != ts::NODE_BINARY_OPERATION) {
        throw std::runtime_error("not a binary_operation node");
    }
    assert(node.child_count() == 3);
}
auto BinaryOperation::left() -> Expression { return Expression(bin_op.child(0).value()); }
auto BinaryOperation::right() -> Expression { return Expression(bin_op.child(2).value()); }
auto BinaryOperation::op() -> BinOpEnum {
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
        return BinOpEnum::BSL;
    } else if (op_str == ">>"s) {
        return BinOpEnum::BSR;
    } else if (op_str == "//"s) {
        return BinOpEnum::INTDIV;
    } else if (op_str == "|"s) {
        return BinOpEnum::BWOR;
    } else if (op_str == "&"s) {
        return BinOpEnum::BWAND;
    } else if (op_str == "~"s) {
        return BinOpEnum::BWNOT;
    } else {
        throw std::runtime_error("Unknown Binary Operator: " + op_str);
    }
}
auto BinaryOperation::raw() -> ts::Node { return this->bin_op; }
// UnaryOperation
UnaryOperation::UnaryOperation(ts::Node node) : un_op(node) {
    if (node.type_id() != ts::NODE_UNARY_OPERATION) {
        throw std::runtime_error("not an unary_operation node");
    }
    assert(node.child_count() == 2);
}

auto UnaryOperation::op() -> UnOpEnum {
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

auto UnaryOperation::exp() -> Expression { return Expression(un_op.child(0).value()); }
auto UnaryOperation::raw() -> ts::Node { return this->un_op; }

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
auto ForStatement::body() -> Body {
    std::vector<ts::Node> body = for_statement.named_children();
    body.erase(body.begin());
    return Body(body);
}

auto ForStatement::loop_exp() -> LoopExpression {
    return LoopExpression(for_statement.named_child(0).value());
}
auto ForStatement::raw() -> ts::Node { return this->for_statement; }

LoopExpression::LoopExpression(ts::Node node) : loop_exp(node) {
    if (node.type_id() != ts::NODE_LOOP_EXPRESSION) {
        throw std::runtime_error("not a loop_expression node");
    }
    assert(node.named_child_count() == 3 || node.named_child_count() == 4);
}
auto LoopExpression::variable() -> Identifier {
    return Identifier(loop_exp.named_child(0).value());
}
auto LoopExpression::end() -> Expression {
    if (loop_exp.named_child_count() == 4) {
        return Expression(loop_exp.named_child(3).value());
    } else {
        return Expression(loop_exp.named_child(2).value());
    }
}
auto LoopExpression::start() -> Expression { return Expression(loop_exp.named_child(1).value()); }
auto LoopExpression::step() -> std::optional<Expression> {
    if (loop_exp.named_child_count() == 4) {
        return Expression(loop_exp.named_child(2).value());
    } else {
        return {};
    }
}
auto LoopExpression::raw() -> ts::Node { return this->loop_exp; }

InLoopExpression::InLoopExpression(ts::Node node) : loop_exp(node) {
    if (node.type_id() != ts::NODE_LOOP_EXPRESSION) {
        throw std::runtime_error("not a in_loop_expression node");
    }
    assert(node.named_child_count() == 2);
}
auto InLoopExpression::loop_exps() -> std::vector<Expression> {
    std::vector<ts::Node> children = loop_exp.named_child(1).value().named_children();
    std::vector<Expression> exps;
    exps.reserve(children.size());
    std::transform(children.begin(), children.end(), std::back_inserter(exps), [](ts::Node node) {
        return Expression(node);
    });
    return exps;
}
auto InLoopExpression::loop_vars() -> std::vector<Identifier> {
    std::vector<ts::Node> children = loop_exp.named_child(0).value().named_children();
    std::vector<Identifier> loop_vars;
    loop_vars.reserve(children.size());
    std::transform(
        children.begin(), children.end(), std::back_inserter(loop_vars),
        [](ts::Node node) { return Identifier(node); });
    return loop_vars;
}
auto InLoopExpression::raw() -> ts::Node { return this->loop_exp; }

// ForInStatement
ForInStatement::ForInStatement(ts::Node node) : for_in(node) {
    if (node.type_id() != ts::NODE_FOR_IN_STATEMENT) {
        throw std::runtime_error("not a for_in_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0)->type_id() == ts::NODE_LOOP_EXPRESSION);
}
auto ForInStatement::loop_exp() -> InLoopExpression {
    return InLoopExpression(for_in.named_child(0).value());
}
auto ForInStatement::body() -> Body {
    std::vector<ts::Node> children = for_in.named_children();
    children.erase(children.begin());
    return Body(children);
}
auto ForInStatement::raw() -> ts::Node { return this->for_in; }

// WhileStatement
WhileStatement::WhileStatement(ts::Node node) : while_statement(node) {
    if (node.type_id() != ts::NODE_WHILE_STATEMENT) {
        throw std::runtime_error("not a while_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}

auto WhileStatement::body() -> Body {
    std::vector<ts::Node> body = while_statement.named_children();
    body.erase(body.begin());
    return Body(body);
}
auto WhileStatement::exit_cond() -> Expression {
    return Expression(while_statement.named_child(0).value().named_child(0).value());
}
auto WhileStatement::raw() -> ts::Node { return this->while_statement; }

RepeatStatement::RepeatStatement(ts::Node node) : repeat_statement(node) {
    if (node.type_id() != ts::NODE_REPEAT_STATEMENT) {
        throw std::runtime_error("not a repeat_statement node");
    }
    assert(
        node.named_child_count() >= 1 &&
        (node.named_children().end() - 1)->type_id() == ts::NODE_CONDITION_EXPRESSION);
}
auto RepeatStatement::body() -> Body {
    std::vector<ts::Node> body = repeat_statement.named_children();
    body.pop_back();
    return Body(body);
}
auto RepeatStatement::until_cond() -> Expression {
    return Expression(repeat_statement.named_child(repeat_statement.named_child_count() - 1)
                          .value()
                          .named_child(0)
                          .value());
}
auto RepeatStatement::raw() -> ts::Node { return this->repeat_statement; }

// If
IfStatement::IfStatement(ts::Node node) : if_statement(node) {
    if (node.type_id() != ts::NODE_IF_STATEMENT) {
        throw std::runtime_error("not an if_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}
auto IfStatement::cond() -> Expression {
    return Expression(if_statement.named_child(0).value().named_child(0).value());
}
auto IfStatement::else_() -> std::optional<Else> {
    if (if_statement.named_child(if_statement.named_child_count() - 1).has_value() &&
        if_statement.named_child(if_statement.named_child_count() - 1)->type_id() ==
            ts::NODE_ELSE) {
        return Else(if_statement.named_child(if_statement.named_child_count() - 1).value());
    } else {
        return {};
    }
}
auto IfStatement::elseifs() -> std::vector<ElseIf> {
    if (if_statement.named_child_count() == 1) {
        return std::vector<ElseIf>();
    }
    std::vector<ts::Node> if_children = if_statement.named_children();
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
auto IfStatement::body() -> Body {
    if (if_statement.named_child_count() == 1 ||
        (if_statement.named_child(1).has_value() &&
         if_statement.named_child(1)->type_id() == ts::NODE_ELSEIF)) {
        return Body(std::vector<ts::Node>());
    }
    std::vector<ts::Node> if_children = if_statement.named_children();
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
auto IfStatement::raw() -> ts::Node { return this->if_statement; }

// Else
Else::Else(ts::Node node) : else_statement(node) {
    if (node.type_id() != ts::NODE_ELSE) {
        throw std::runtime_error("Not an else_statement node");
    }
}
auto Else::body() -> Body { return Body(else_statement.named_children()); }
auto Else::raw() -> ts::Node { return this->else_statement; }

// ElseIf
ElseIf::ElseIf(ts::Node node) : else_if(node) {
    if (node.type_id() != ts::NODE_ELSEIF) {
        throw std::runtime_error("not a else_if node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}
auto ElseIf::body() -> Body {
    std::vector<ts::Node> body = else_if.named_children();
    body.erase(body.begin());
    return Body(body);
}
auto ElseIf::cond() -> Expression {
    return Expression(else_if.named_child(0).value().named_child(0).value());
}
auto ElseIf::raw() -> ts::Node { return this->else_if; }

// Return
Return::Return(ts::Node node) : expressions(node) {}
auto Return::explist() -> std::vector<Expression> {
    std::vector<ts::Node> exps = expressions.named_children();
    std::vector<ts::Node>::iterator end;
    std::vector<Expression> res;
    if (exps.size() > 1 && exps[exps.size() - 1].text() == ";"s) {
        end = exps.end() - 1;
        res.reserve(exps.size() - 1);
    } else {
        end = exps.end();
        res.reserve(exps.size());
    }
    std::transform(
        exps.begin(), end, std::back_inserter(res), [](ts::Node node) { return Expression(node); });
    return res;
}
auto Return::raw() -> ts::Node { return this->expressions; }

// VariableDeclaration
VariableDeclaration::VariableDeclaration(ts::Node node) : var_dec(node) {
    if (node.type_id() != ts::NODE_VARIABLE_DECLARATION &&
        node.type_id() != ts::NODE_LOCAL_VARIABLE_DECLARATION) {
        throw std::runtime_error("not a variable_declaration node");
    } else {
        local_dec = node.type_id() == ts::NODE_LOCAL_VARIABLE_DECLARATION;
    }
}
auto VariableDeclaration::declarations() -> std::vector<Expression> {
    std::vector<Expression> res;
    if (local_dec) {
        std::vector<ts::Node> nodes = var_dec.named_children();
        res.reserve(nodes.size() - 1);
        std::transform(nodes.begin() + 1, nodes.end(), std::back_inserter(res), [](ts::Node node) {
            return Expression(node);
        });
        return res;
    } else {
        std::vector<ts::Node> nodes = var_dec.named_children();
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
auto VariableDeclaration::declarators() -> std::vector<VariableDeclarator> {
    if (local_dec) {
        std::vector<ts::Node> nodes = var_dec.named_child(0).value().named_children();
        std::vector<VariableDeclarator> res;
        res.reserve(nodes.size());
        std::transform(nodes.begin(), nodes.end(), std::back_inserter(res), [](ts::Node node) {
            return VariableDeclarator(node);
        });
        return res;
    } else {
        std::vector<ts::Node> nodes = var_dec.named_children();
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
auto VariableDeclaration::local() -> bool { return local_dec; }
auto VariableDeclaration::raw() -> ts::Node { return this->var_dec; }

// VariableDeclarator
VariableDeclarator::VariableDeclarator(ts::Node node) : dec(node) {
    if (node.type_id() != ts::NODE_VARIABLE_DECLARATOR && node.type_id() != ts::NODE_IDENTIFIER &&
        node.type_id() != ts::NODE_FIELD_EXPRESSION && node.type_id() != ts::NODE_TABLE_INDEX) {
        throw std::runtime_error("not a variable declarator");
    }
}
auto VariableDeclarator::var() -> std::variant<Identifier, FieldExpression, TableIndex> {
    if (dec.type_id() == ts::NODE_IDENTIFIER) {
        return Identifier(dec);
    } else if (dec.type_id() == ts::NODE_FIELD_EXPRESSION) {
        return FieldExpression(dec);
    } else if (dec.type_id() == ts::NODE_TABLE_INDEX) {
        return TableIndex(dec);
    }
    if (dec.named_child(0).has_value()) {
        if (dec.named_child(0)->type_id() == ts::NODE_IDENTIFIER) {
            return Identifier(dec.named_child(0).value());
        } else if (dec.named_child(0)->type_id() == ts::NODE_FIELD_EXPRESSION) {
            return FieldExpression(dec.named_child(0).value());
        } else if (dec.named_child(0)->type_id() == ts::NODE_TABLE_INDEX) {
            return TableIndex(dec.named_child(0).value());
        } else {
            throw std::runtime_error("invalid variable declarator");
        }
    } else {
        throw std::runtime_error("invalid variable declarator");
    }
}
auto VariableDeclarator::raw() -> ts::Node { return this->dec; }

// TableIndex
TableIndex::TableIndex(ts::Node node) : table_index(node) {
    if (node.type_id() != ts::NODE_TABLE_INDEX) {
        throw std::runtime_error("not a table_index node");
    }
    assert(node.named_child_count() == 2);
}
auto TableIndex::table() -> Prefix { return Prefix(table_index.named_child(0).value()); }
auto TableIndex::index() -> Expression { return Expression(table_index.named_child(1).value()); }
auto TableIndex::raw() -> ts::Node { return this->table_index; }

// DoStatement
DoStatement::DoStatement(ts::Node node) : do_statement(node) {
    if (node.type_id() != ts::NODE_DO_STATEMENT) {
        throw std::runtime_error("not a do_statement node");
    }
}
auto DoStatement::body() -> Body { return Body(do_statement.named_children()); }
auto DoStatement::raw() -> ts::Node { return this->do_statement; }

// FieldExpression
FieldExpression::FieldExpression(ts::Node node) : exp(node) {
    if (node.type_id() != ts::NODE_FIELD_EXPRESSION) {
        throw std::runtime_error("not a field_expression node");
    }
    assert(node.named_child_count() == 2);
}
auto FieldExpression::table_id() -> Prefix { return Prefix(exp.named_child(0).value()); }
auto FieldExpression::property_id() -> Identifier { return Identifier(exp.named_child(1).value()); }
auto FieldExpression::raw() -> ts::Node { return this->exp; }

// Label
Label::Label(ts::Node node) : label(node) {
    if (node.type_id() != ts::NODE_LABEL_STATEMENT) {
        throw std::runtime_error("not a label node");
    }
    assert(node.named_child_count() == 1);
}
auto Label::id() -> Identifier { return Identifier(label.named_child(0).value()); }
auto Label::raw() -> ts::Node { return this->label; }

// GoTo
GoTo::GoTo(ts::Node node) : go_to(node) {
    if (node.type_id() != ts::NODE_GOTO_STATEMENT) {
        throw std::runtime_error("not a go_to node");
    }
    assert(node.named_child_count() == 1);
}
auto GoTo::label() -> Identifier { return Identifier(go_to.named_child(0).value()); }
auto GoTo::raw() -> ts::Node { return this->go_to; }

// Parameters
Parameters::Parameters(ts::Node node) : parameters(node) {
    if (node.type_id() != ts::NODE_PARAMETERS) {
        throw std::runtime_error("not a parameters node");
    }
}
auto Parameters::leading_self() -> bool {
    if (parameters.named_child_count() < 1) {
        return false;
    } else {
        return parameters.named_child(0)->type_id() == ts::NODE_SELF;
    }
}
auto Parameters::spread() -> SpreadPos {
    if (parameters.named_child_count() < 1) {
        return SpreadPos::NO_SPREAD;
    } else if (parameters.named_child(0)->type_id() == ts::NODE_SPREAD) {
        return SpreadPos::BEGIN;
    } else if (
        parameters.named_child(parameters.named_child_count() - 1)->type_id() == ts::NODE_SPREAD) {
        return SpreadPos::END;
    } else {
        return SpreadPos::NO_SPREAD;
    }
}
auto Parameters::params() -> std::vector<Identifier> {
    SpreadPos sp = spread();
    std::vector<Identifier> res;
    std::vector<ts::Node> children = parameters.named_children();
    if (leading_self()) {
        if (sp == NO_SPREAD) {
            if (parameters.named_child_count() < 1) {
                return res;
            }
            res.reserve(parameters.named_child_count() - 1);
            std::transform(
                children.begin() + 1, children.end(), std::back_inserter(res),
                [](ts::Node node) { return Identifier(node); });
        } else {
            if (parameters.named_child_count() < 2) {
                return res;
            }
            res.reserve(parameters.named_child_count() - 2);
            std::transform(
                children.begin() + 1, children.end() - 1, std::back_inserter(res),
                [](ts::Node node) { return Identifier(node); });
        }
    } else {
        switch (sp) {
        case NO_SPREAD:
            res.reserve(parameters.named_child_count());
            std::transform(
                children.begin(), children.end(), std::back_inserter(res),
                [](ts::Node node) { return Identifier(node); });
            break;
        case BEGIN:
            if (parameters.named_child_count() < 1) {
                return res;
            }
            res.reserve(parameters.named_child_count() - 1);
            std::transform(
                children.begin() + 1, children.end(), std::back_inserter(res),
                [](ts::Node node) { return Identifier(node); });
            break;
        case END:
            if (parameters.named_child_count() < 1) {
                return res;
            }
            res.reserve(parameters.named_child_count() - 1);
            std::transform(
                children.begin(), children.end() - 1, std::back_inserter(res),
                [](ts::Node node) { return Identifier(node); });
            break;
        }
    }
    return res;
}
auto Parameters::raw() -> ts::Node { return this->parameters; }

// FunctionName
FunctionName::FunctionName(ts::Node node) : func_name(node) {
    if (node.type_id() != ts::NODE_FUNCTION_NAME) {
        throw std::runtime_error("Not a function_name node");
    }
}
auto FunctionName::method() -> std::optional<Identifier> {
    if (func_name.named_child(func_name.named_child_count() - 1)->type_id() == ts::NODE_METHOD) {
        return Identifier(func_name.named_child(func_name.named_child_count() - 1).value());
    } else {
        return {};
    }
}
auto FunctionName::identifier() -> std::vector<Identifier> {
    if (func_name.named_child(0)->type_id() == ts::NODE_IDENTIFIER) {
        return std::vector<Identifier>{Identifier(func_name.named_child(0).value())};
    } else {
        std::vector<Identifier> res;
        std::vector<ts::Node> children = func_name.named_child(0).value().named_children();
        res.reserve(children.size());
        std::transform(
            children.begin(), children.end(), std::back_inserter(res),
            [](ts::Node node) { return Identifier(node); });
        return res;
    }
}
auto FunctionName::raw() -> ts::Node { return this->func_name; }

// FunctionDefinition
FunctionDefinition::FunctionDefinition(ts::Node node) : func_def(node) {
    if (node.type_id() != ts::NODE_FUNCTION_DEFINITION) {
        throw std::runtime_error("not a function_definition node");
    }
}
auto FunctionDefinition::parameters() -> Parameters {
    return Parameters(func_def.named_child(0).value());
}
auto FunctionDefinition::body() -> Body {
    std::vector<ts::Node> children = func_def.named_children();
    return Body(std::vector<ts::Node>(children.begin() + 1, children.end()));
}
auto FunctionDefinition::raw() -> ts::Node { return this->func_def; }

// FunctionStatement
FunctionStatement::FunctionStatement(ts::Node node) : func_stat(node) {
    if (node.type_id() != ts::NODE_FUNCTION) {
        throw std::runtime_error("not a function(_statement) node");
    }
}
auto FunctionStatement::body() -> Body {
    std::vector<ts::Node> children = func_stat.named_children();
    return Body(std::vector<ts::Node>(children.begin() + 2, children.end()));
}
auto FunctionStatement::name() -> FunctionName {
    return FunctionName(func_stat.named_child(0).value());
}
auto FunctionStatement::parameters() -> Parameters {
    return Parameters(func_stat.named_child(1).value());
}
auto FunctionStatement::raw() -> ts::Node { return this->func_stat; }

// LocalFunctionStatement
LocalFunctionStatement::LocalFunctionStatement(ts::Node node) : func_stat(node) {
    if (node.type_id() != ts::NODE_LOCAL_FUNCTION) {
        throw std::runtime_error("not a function(_statement) node");
    }
}
auto LocalFunctionStatement::parameters() -> Parameters {
    return Parameters(func_stat.named_child(1).value());
}
auto LocalFunctionStatement::name() -> Identifier {
    return Identifier(func_stat.named_child(0).value());
}
auto LocalFunctionStatement::body() -> Body {
    std::vector<ts::Node> children = func_stat.named_children();
    return Body(std::vector<ts::Node>(children.begin() + 2, children.end()));
}
auto LocalFunctionStatement::raw() -> ts::Node { return this->func_stat; }

// FunctionCall
FunctionCall::FunctionCall(ts::Node node) : func_call(node) {
    if (node.type_id() != ts::NODE_FUNCTION_CALL) {
        throw runtime_error("not a function_call node");
    }
    assert(node.named_child_count() == 2 || node.named_child_count() == 3);
}
auto FunctionCall::method() -> std::optional<Identifier> {
    if (func_call.named_child_count() == 3) {
        return Identifier(func_call.named_child(1).value());
    } else {
        return {};
    }
}
auto FunctionCall::id() -> Prefix { return Prefix(func_call.named_child(0).value()); }
auto FunctionCall::args() -> std::vector<Expression> {
    std::vector<ts::Node> arg_nodes =
        func_call.named_child(func_call.named_child_count() - 1).value().named_children();
    std::vector<Expression> args;
    args.reserve(arg_nodes.size());
    std::transform(arg_nodes.begin(), arg_nodes.end(), std::back_inserter(args), [](ts::Node node) {
        return Expression(node);
    });
    return args;
}
auto FunctionCall::raw() -> ts::Node { return this->func_call; }

// GlobalVariable
GlobalVariable::GlobalVariable(ts::Node node) : g_var(node) {
    if (node.type_id() != ts::NODE_GLOBAL_VARIABLE ||
        !(node.text() == "_G" || node.text() == "_VERSION")) {
        throw runtime_error("not a global variable node");
    }
}
auto GlobalVariable::type() -> GV {
    if (g_var.text() == "_G") {
        return GV::_G;
    } else {
        return GV::_VERSION;
    }
}
auto GlobalVariable::raw() -> ts::Node { return this->g_var; }

// Table
Table::Table(ts::Node node) : table(node) {
    if (node.type_id() != ts::NODE_TABLE) {
        throw runtime_error("not a table node");
    }
}
auto Table::fields() -> std::vector<Field> {
    if (table.named_child_count() < 1) {
        return std::vector<Field>();
    }
    std::vector<Field> fields;
    fields.reserve(table.named_child_count());
    std::vector<ts::Node> nodes = table.named_children();
    std::transform(nodes.begin(), nodes.end(), std::back_inserter(fields), [](ts::Node node) {
        return Field(node);
    });
    return fields;
}
auto Table::raw() -> ts::Node { return this->table; }

// Field
Field::Field(ts::Node node) : field(node) {
    if (node.type_id() != ts::NODE_FIELD) {
        throw runtime_error("not a field node");
    }
    assert(node.named_child_count() == 1 || node.named_child_count() == 2);
}
auto Field::content() -> std::variant<
    std::pair<Expression, Expression>, std::pair<Identifier, Expression>, Expression> {
    if (field.named_child_count() < 2) {
        return Expression(field.named_child(0).value());
    } else if (field.child(0)->text() == "[") {
        return std::pair<Expression, Expression>(
            Expression(field.named_child(0).value()), Expression(field.named_child(1).value()));
    } else {
        return std::pair<Identifier, Expression>(
            Identifier(field.named_child(0).value()), Expression(field.named_child(1).value()));
    }
}
auto Field::raw() -> ts::Node { return this->field; }

// Prefix
Prefix::Prefix(ts::Node node) : prefix(node) {
    if (!(node.type_id() == ts::NODE_SELF || node.type_id() == ts::NODE_GLOBAL_VARIABLE ||
          node.type_id() == ts::NODE_FUNCTION_CALL || node.type_id() == ts::NODE_IDENTIFIER ||
          node.type_id() == ts::NODE_FIELD_EXPRESSION || node.type_id() == ts::NODE_TABLE_INDEX)) {
        throw std::runtime_error("Not a prefix-node");
    }
}
auto Prefix::options()
    -> std::variant<Self, GlobalVariable, VariableDeclarator, FunctionCall, Expression> {
    if (prefix.type_id() == ts::NODE_SELF) {
        return Self();
    } else if (prefix.type_id() == ts::NODE_GLOBAL_VARIABLE) {
        return GlobalVariable(prefix);
    } else if (prefix.type_id() == ts::NODE_FUNCTION_CALL) {
        return FunctionCall(prefix);
    } else if (prefix.child_count() > 0 && prefix.child(0)->text() == "(") {
        return Expression(prefix.child(1).value());
    } else if (
        prefix.type_id() == ts::NODE_IDENTIFIER || prefix.type_id() == ts::NODE_FIELD_EXPRESSION ||
        prefix.type_id() == ts::NODE_TABLE_INDEX) {
        return VariableDeclarator(prefix);
    } else {
        throw std::runtime_error("Not a prefix-node");
    }
}
auto Prefix::raw() -> ts::Node { return this->prefix; }

// Expression
Expression::Expression(ts::Node node) : exp(node) {
    if (!(node.type_id() == ts::NODE_SPREAD || node.type_id() == ts::NODE_NEXT ||
          node.type_id() == ts::NODE_FUNCTION_DEFINITION || node.type_id() == ts::NODE_TABLE ||
          node.type_id() == ts::NODE_BINARY_OPERATION ||
          node.type_id() == ts::NODE_UNARY_OPERATION || node.type_id() == ts::NODE_STRING ||
          node.type_id() == ts::NODE_NUMBER || node.type_id() == ts::NODE_NIL ||
          node.type_id() == ts::NODE_FALSE || node.type_id() == ts::NODE_TRUE ||
          node.type_id() == ts::NODE_IDENTIFIER ||
          (node.type_id() == ts::NODE_SELF || node.type_id() == ts::NODE_GLOBAL_VARIABLE ||
           node.type_id() == ts::NODE_FUNCTION_CALL || node.child(0)->text() == "("))) {
        throw std::runtime_error("Not an expression-node");
    }
}
auto Expression::options() -> std::variant<
    Spread, Prefix, Next, FunctionDefinition, Table, BinaryOperation, UnaryOperation,
    minilua::Value, Identifier> {
    if (exp.type_id() == ts::NODE_SPREAD) {
        return Spread();
    } else if (exp.type_id() == ts::NODE_NEXT) {
        return Next();
    } else if (exp.type_id() == ts::NODE_FUNCTION_DEFINITION) {
        return FunctionDefinition(exp);
    } else if (exp.type_id() == ts::NODE_TABLE) {
        return Table(exp);
    } else if (exp.type_id() == ts::NODE_BINARY_OPERATION) {
        return BinaryOperation(exp);
    } else if (exp.type_id() == ts::NODE_UNARY_OPERATION) {
        return UnaryOperation(exp);
    } else if (exp.type_id() == ts::NODE_STRING) {
        std::string str = exp.text();
        return minilua::Value(String(std::string(str.begin() + 1, str.end() - 1)));
    } else if (exp.type_id() == ts::NODE_NUMBER) {
        return parse_number_literal(exp.text());
    } else if (exp.type_id() == ts::NODE_NIL) {
        return minilua::Value(Value::Type(Nil()));
    } else if (exp.type_id() == ts::NODE_TRUE) {
        return minilua::Value(true);
    } else if (exp.type_id() == ts::NODE_FALSE) {
        return minilua::Value(false);
    } else if (exp.type_id() == ts::NODE_IDENTIFIER) {
        return Identifier(exp);
    } else if (
        exp.type_id() == ts::NODE_SELF || exp.type_id() == ts::NODE_FUNCTION_CALL ||
        exp.type_id() == ts::NODE_GLOBAL_VARIABLE || exp.type_id() == ts::NODE_FIELD_EXPRESSION ||
        exp.type_id() == ts::NODE_TABLE_INDEX ||
        (exp.child(0).has_value() && exp.child(0)->text() == "(")) {
        return Prefix(exp);
    } else {
        throw std::runtime_error("Not an expression-node");
    }
}
auto Expression::raw() -> ts::Node { return this->exp; }

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
        throw std::runtime_error("Not a statement-node" + to_string(node.type_id()));
    }
}
auto Statement::options() -> std::variant<
    VariableDeclaration, DoStatement, IfStatement, WhileStatement, RepeatStatement, ForStatement,
    ForInStatement, GoTo, Break, Label, FunctionStatement, LocalFunctionStatement, FunctionCall,
    Expression> {
    if (statement.type_id() == ts::NODE_EXPRESSION) {
        return Expression(statement.named_child(0).value());
    } else if (
        statement.type_id() == ts::NODE_VARIABLE_DECLARATION ||
        statement.type_id() == ts::NODE_LOCAL_VARIABLE_DECLARATION) {
        return VariableDeclaration(statement);
    } else if (statement.type_id() == ts::NODE_DO_STATEMENT) {
        return DoStatement(statement);
    } else if (statement.type_id() == ts::NODE_IF_STATEMENT) {
        return IfStatement(statement);
    } else if (statement.type_id() == ts::NODE_WHILE_STATEMENT) {
        return WhileStatement(statement);
    } else if (statement.type_id() == ts::NODE_REPEAT_STATEMENT) {
        return RepeatStatement(statement);
    } else if (statement.type_id() == ts::NODE_FOR_STATEMENT) {
        return ForStatement(statement);
    } else if (statement.type_id() == ts::NODE_FOR_IN_STATEMENT) {
        return ForInStatement(statement);
    } else if (statement.type_id() == ts::NODE_GOTO_STATEMENT) {
        return GoTo(statement);
    } else if (statement.type_id() == ts::NODE_BREAK_STATEMENT) {
        return Break();
    } else if (statement.type_id() == ts::NODE_LABEL_STATEMENT) {
        return Label(statement);
    } else if (statement.type_id() == ts::NODE_FUNCTION) {
        return FunctionStatement(statement);
    } else if (statement.type_id() == ts::NODE_LOCAL_FUNCTION) {
        return LocalFunctionStatement(statement);
    } else if (statement.type_id() == ts::NODE_FUNCTION_CALL) {
        return FunctionCall(statement);
    } else {
        throw std::runtime_error("Not a statement-node");
    }
}
auto Statement::raw() -> ts::Node { return this->statement; }

} // namespace minilua::details::ast
