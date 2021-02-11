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
Body::Body(std::vector<ts::Node> node_vec) : content(std::move(node_vec)) {}
Body::Body(std::vector<Statement> stats, std::optional<Return> ret)
    : content(std::make_pair<>(std::move(stats), ret)) {}
auto Body::return_statement() -> std::optional<Return> {
    return std::visit(
        overloaded{
            [](std::vector<ts::Node> nodes) -> std::optional<Return> {
                if (nodes.empty() ||
                    nodes[nodes.size() - 1].type_id() != ts::NODE_RETURN_STATEMENT) {
                    return std::nullopt;
                } else {
                    return Return(nodes[nodes.size() - 1]);
                }
            },
            [](const BodyPair& pair) -> std::optional<Return> { return pair.second; }},
        this->content);
}
auto Body::statements() -> std::vector<Statement> {
    return std::visit(
        overloaded{
            [](std::vector<ts::Node> nodes) -> std::vector<Statement> {
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
                std::transform(nodes.begin(), end, std::back_inserter(res), [](ts::Node node) {
                    return Statement(node);
                });
                return res;
            },
            [](const BodyPair& pair) -> std::vector<Statement> { return pair.first; }},
        this->content);
}
// Identifier
Identifier::Identifier(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_IDENTIFIER && node.type_id() != ts::NODE_METHOD &&
        node.type_id() != ts::NODE_PROPERTY_IDENTIFIER &&
        node.type_id() != ts::NODE_FUNCTION_NAME_FIELD) {
        throw std::runtime_error("not an identifier node" + to_string(node.type_id()));
    }
}
Identifier::Identifier(const std::string& str, Range range)
    : content(std::make_tuple(str, range)) {}
auto Identifier::string() const -> std::string {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::string { return node.text(); },
            [](IdTuple tuple) -> std::string { return std::get<STRING>(tuple); }},
        this->content);
}
auto Identifier::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](IdTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto Identifier::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const IdTuple& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}

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
BinaryOperation::BinaryOperation(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_BINARY_OPERATION) {
        throw std::runtime_error("not a binary_operation node");
    }
    assert(node.child_count() == 3);
}
BinaryOperation::BinaryOperation(
    const Expression& left, BinOpEnum op_enum, const Expression& right, minilua::Range range)
    : content(std::make_tuple(
          std::make_shared<Expression>(left), op_enum, std::make_shared<Expression>(right),
          range)) {}
auto BinaryOperation::left() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression { return Expression(node.child(0).value()); },
            [](BinOpTuple tuple) -> Expression { return *std::get<LEFT>(tuple); }},
        this->content);
}
auto BinaryOperation::right() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression { return Expression(node.child(2).value()); },
            [](BinOpTuple tuple) -> Expression { return *std::get<RIGHT>(tuple); }},
        this->content);
}
auto BinaryOperation::binary_operator() const -> BinOpEnum {
    return std::visit(
        overloaded{
            [](ts::Node node) -> BinOpEnum {
                std::string op_str = node.child(1)->text();
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
            },
            [](BinOpTuple tuple) -> BinOpEnum { return std::get<OPERATOR>(tuple); }},
        this->content);
}
auto BinaryOperation::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](BinOpTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto BinaryOperation::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const BinOpTuple& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}
// UnaryOperation
UnaryOperation::UnaryOperation(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_UNARY_OPERATION) {
        throw std::runtime_error("not an unary_operation node");
    }
    assert(node.child_count() == 2);
}
UnaryOperation::UnaryOperation(UnOpEnum op_enum, const Expression& exp, minilua::Range range)
    : content(std::make_tuple(op_enum, std::make_shared<Expression>(exp), range)) {}
auto UnaryOperation::unary_operator() const -> UnOpEnum {
    return std::visit(
        overloaded{
            [](ts::Node node) -> UnOpEnum {
                if (node.child(0)->text() == "not"s) {
                    return UnOpEnum::NOT;
                } else if (node.child(0)->text() == "-"s) {
                    return UnOpEnum::NEG;
                } else if (node.child(0)->text() == "~"s) {
                    return UnOpEnum::BWNOT;
                } else if (node.child(0)->text() == "#"s) {
                    return UnOpEnum::LEN;
                } else {
                    throw std::runtime_error("unknown Unary Operator: " + node.child(0)->text());
                }
            },
            [](UnOpTuple tuple) -> UnOpEnum { return std::get<OPERATOR>(tuple); }},
        this->content);
}

auto UnaryOperation::expression() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression { return Expression(node.child(1).value()); },
            [](UnOpTuple tuple) -> Expression { return *std::get<OPERAND>(tuple); }},
        this->content);
}
auto UnaryOperation::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](UnOpTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto UnaryOperation::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const UnOpTuple& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}

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
WhileStatement::WhileStatement(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_WHILE_STATEMENT) {
        throw std::runtime_error("not a while_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}
WhileStatement::WhileStatement(const Expression& cond, const Body& body, minilua::Range range)
    : content(std::make_tuple(
          std::make_shared<Expression>(cond), std::make_shared<Body>(body), range)) {}
auto WhileStatement::body() const -> Body {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Body {
                std::vector<ts::Node> body = node.named_children();
                body.erase(body.begin());
                return Body(body);
            },
            [](WhileTuple tuple) -> Body { return *std::get<BODY>(tuple); }},
        this->content);
}
auto WhileStatement::repeat_conditon() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression {
                return Expression(node.named_child(0).value().named_child(0).value());
            },
            [](WhileTuple tuple) -> Expression { return *std::get<CONDITION>(tuple); }},
        this->content);
}

auto WhileStatement::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](WhileTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto WhileStatement::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const WhileTuple& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}

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
IfStatement::IfStatement(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_IF_STATEMENT) {
        throw std::runtime_error("not an if_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}
IfStatement::IfStatement(const Expression& cond, const Body& body, minilua::Range range)
    : content(std::make_tuple(
          std::make_shared<Expression>(cond), std::make_shared<Body>(body), range)) {}
auto IfStatement::condition() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression {
                return Expression(node.named_child(0).value().named_child(0).value());
            },
            [](IfTuple tuple) -> Expression { return *std::get<CONDITION>(tuple); }},
        this->content);
}
auto IfStatement::else_statement() const -> std::optional<Else> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<Else> {
                if (node.named_child(node.named_child_count() - 1).has_value() &&
                    node.named_child(node.named_child_count() - 1)->type_id() == ts::NODE_ELSE) {
                    return Else(node.named_child(node.named_child_count() - 1).value());
                } else {
                    return std::nullopt;
                }
            },
            [](const IfTuple& /*tuple*/) -> std::optional<Else> { return nullopt; }},
        this->content);
}
auto IfStatement::elseifs() const -> std::vector<ElseIf> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::vector<ElseIf> {
                if (node.named_child_count() == 1) {
                    return std::vector<ElseIf>();
                }
                std::vector<ts::Node> if_children = node.named_children();
                std::vector<ts::Node>::iterator end;
                std::vector<ElseIf> res;
                if ((if_children.end() - 1)->type_id() == ts::NODE_ELSE) {
                    end = if_children.end() - 1;
                } else {
                    end = if_children.end();
                }
                auto begin = end;
                while ((begin - 1)->type_id() == ts::NODE_ELSEIF &&
                       begin - 1 != if_children.begin()) {
                    begin--;
                }
                if (begin == end) {
                    return std::vector<ElseIf>();
                }
                std::transform(begin, end, std::back_inserter(res), [](ts::Node node) {
                    return ElseIf(node);
                });
                return res;
            },
            [](const IfTuple& /*tuple*/) -> std::vector<ElseIf> { return std::vector<ElseIf>(); }},
        this->content);
}
auto IfStatement::body() const -> Body {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Body {
                if (node.named_child_count() == 1 ||
                    (node.named_child(1).has_value() &&
                     node.named_child(1)->type_id() == ts::NODE_ELSEIF)) {
                    return Body(std::vector<ts::Node>());
                }
                std::vector<ts::Node> if_children = node.named_children();
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
            },
            [](IfTuple tuple) -> Body { return *std::get<BODY>(tuple); }},
        this->content);
}
auto IfStatement::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](IfTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto IfStatement::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const IfTuple& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}

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
VariableDeclaration::VariableDeclaration(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_VARIABLE_DECLARATION &&
        node.type_id() != ts::NODE_LOCAL_VARIABLE_DECLARATION) {
        throw std::runtime_error("not a variable_declaration node");
    } else {
        local_dec = node.type_id() == ts::NODE_LOCAL_VARIABLE_DECLARATION;
    }
}
VariableDeclaration::VariableDeclaration(
    bool local, const std::vector<VariableDeclarator>& v_declarators,
    const std::vector<Expression>& v_declarations, minilua::Range range)
    : content(std::make_tuple(v_declarators, v_declarations, range)), local_dec(local) {}
auto VariableDeclaration::declarations() const -> std::vector<Expression> {
    return std::visit(
        overloaded{
            [this](ts::Node node) -> std::vector<Expression> {
                std::vector<Expression> res;
                if (this->local_dec) {
                    std::vector<ts::Node> nodes = node.named_children();
                    res.reserve(nodes.size() - 1);
                    std::transform(
                        nodes.begin() + 1, nodes.end(), std::back_inserter(res),
                        [](ts::Node node) { return Expression(node); });
                    return res;
                } else {
                    std::vector<ts::Node> nodes = node.named_children();
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
            },
            [](VDTuple tuple) -> std::vector<Expression> { return std::get<DECLARATIONS>(tuple); }},
        this->content);
}
auto VariableDeclaration::declarators() const -> std::vector<VariableDeclarator> {
    return std::visit(
        overloaded{
            [this](ts::Node node) -> std::vector<VariableDeclarator> {
                if (this->local_dec) {
                    std::vector<ts::Node> nodes = node.named_child(0).value().named_children();
                    std::vector<VariableDeclarator> res;
                    res.reserve(nodes.size());
                    std::transform(
                        nodes.begin(), nodes.end(), std::back_inserter(res),
                        [](ts::Node node) { return VariableDeclarator(node); });
                    return res;
                } else {
                    std::vector<ts::Node> nodes = node.named_children();
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
            },
            [](VDTuple tuple) -> std::vector<VariableDeclarator> {
                return std::get<DECLARATORS>(tuple);
            }},
        this->content);
}
auto VariableDeclaration::local() const -> bool { return this->local_dec; }
auto VariableDeclaration::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](VDTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto VariableDeclaration::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const VDTuple& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}

// VariableDeclarator
VariableDeclarator::VariableDeclarator(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_VARIABLE_DECLARATOR && node.type_id() != ts::NODE_IDENTIFIER &&
        node.type_id() != ts::NODE_FIELD_EXPRESSION && node.type_id() != ts::NODE_TABLE_INDEX) {
        throw std::runtime_error("not a variable declarator");
    }
}
VariableDeclarator::VariableDeclarator(const Identifier& id, minilua::Range range)
    : content(std::make_tuple(id, range)) {}
auto VariableDeclarator::options() const -> std::variant<Identifier, FieldExpression, TableIndex> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> VarDecVariant {
                if (node.type_id() == ts::NODE_IDENTIFIER) {
                    return Identifier(node);
                } else if (node.type_id() == ts::NODE_FIELD_EXPRESSION) {
                    return FieldExpression(node);
                } else if (node.type_id() == ts::NODE_TABLE_INDEX) {
                    return TableIndex(node);
                }
                if (node.named_child(0).has_value()) {
                    if (node.named_child(0)->type_id() == ts::NODE_IDENTIFIER) {
                        return Identifier(node.named_child(0).value());
                    } else if (node.named_child(0)->type_id() == ts::NODE_FIELD_EXPRESSION) {
                        return FieldExpression(node.named_child(0).value());
                    } else if (node.named_child(0)->type_id() == ts::NODE_TABLE_INDEX) {
                        return TableIndex(node.named_child(0).value());
                    } else {
                        throw std::runtime_error("invalid variable declarator");
                    }
                } else {
                    throw std::runtime_error("invalid variable declarator");
                }
            },
            [](VDTuple tuple) -> VarDecVariant { return std::get<VD_VARIANT>(tuple); }},
        this->content);
}
auto VariableDeclarator::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) { return convert_range(node.range()); },
            [](VDTuple tuple) { return std::get<RANGE>(tuple); }},
        this->content);
}
auto VariableDeclarator::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const VDTuple& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}

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
DoStatement::DoStatement(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_DO_STATEMENT) {
        throw std::runtime_error("not a do_statement node");
    }
}
DoStatement::DoStatement(const Body& body, minilua::Range range)
    : content(std::make_tuple(std::make_shared<Body>(body), range)) {}
auto DoStatement::body() const -> Body {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Body { return Body(node.named_children()); },
            [](const DoTuple& tuple) -> Body { return *std::get<BODY>(tuple); }},
        this->content);
}
auto DoStatement::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const DoTuple& tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto DoStatement::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const DoTuple& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}

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
FunctionDefinition::FunctionDefinition(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_FUNCTION_DEFINITION) {
        throw std::runtime_error("not a function_definition node");
    }
}
FunctionDefinition::FunctionDefinition(Parameters params, const Body& body, minilua::Range range)
    : content(std::tuple(params, std::make_shared<Body>(body), range)) {}
auto FunctionDefinition::parameters() const -> Parameters {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Parameters { return Parameters(node.named_child(0).value()); },
            [](const FuncDefTuple& tuple) -> Parameters { return std::get<PARAMETERS>(tuple); }},
        this->content);
}
auto FunctionDefinition::body() const -> Body {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Body {
                std::vector<ts::Node> children = node.named_children();
                return Body(std::vector<ts::Node>(children.begin() + 1, children.end()));
            },
            [](const FuncDefTuple& tuple) -> Body { return *std::get<BODY>(tuple); }},
        this->content);
}
auto FunctionDefinition::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const FuncDefTuple& tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto FunctionDefinition::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const FuncDefTuple& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}

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
FunctionCall::FunctionCall(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_FUNCTION_CALL) {
        throw runtime_error("not a function_call node");
    }
    assert(node.named_child_count() == 2 || node.named_child_count() == 3);
}
FunctionCall::FunctionCall(
    const Prefix& pfx, const std::optional<Identifier>& method, const std::vector<Expression>& args,
    minilua::Range range)
    : content(std::make_tuple(std::make_shared<Prefix>(pfx), method, args, range)) {}
auto FunctionCall::method() const -> std::optional<Identifier> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<Identifier> {
                if (node.named_child_count() == 3) {
                    return Identifier(node.named_child(1).value());
                } else {
                    return std::nullopt;
                }
            },
            [](FuncCallTuple tuple) { return std::get<METHOD>(tuple); }},
        this->content);
}
auto FunctionCall::id() const -> Prefix {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Prefix { return Prefix(node.named_child(0).value()); },
            [](FuncCallTuple tuple) -> Prefix { return *std::get<PREFIX>(tuple); }},
        this->content);
}
auto FunctionCall::args() const -> std::vector<Expression> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::vector<Expression> {
                std::vector<ts::Node> arg_nodes =
                    node.named_child(node.named_child_count() - 1).value().named_children();
                std::vector<Expression> args;
                args.reserve(arg_nodes.size());
                std::transform(
                    arg_nodes.begin(), arg_nodes.end(), std::back_inserter(args),
                    [](ts::Node node) { return Expression(node); });
                return args;
            },
            [](FuncCallTuple tuple) -> std::vector<Expression> { return std::get<ARGS>(tuple); }},
        this->content);
}
auto FunctionCall::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](FuncCallTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto FunctionCall::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const FuncCallTuple& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}

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
Prefix::Prefix(ts::Node node) : content(node) {
    if (!(node.type_id() == ts::NODE_SELF || node.type_id() == ts::NODE_FUNCTION_CALL ||
          node.type_id() == ts::NODE_IDENTIFIER || node.type_id() == ts::NODE_FIELD_EXPRESSION ||
          node.type_id() == ts::NODE_TABLE_INDEX)) {
        throw std::runtime_error("Not a prefix-node");
    }
}
Prefix::Prefix(const VariableDeclarator& vd, minilua::Range range)
    : content(std::make_tuple(vd, range)) {}
auto Prefix::options() const -> PrefixVariant {
    return std::visit(
        overloaded{
            [](ts::Node node) -> PrefixVariant {
                if (node.type_id() == ts::NODE_SELF) {
                    return Self();
                } else if (node.type_id() == ts::NODE_FUNCTION_CALL) {
                    return FunctionCall(node);
                } else if (node.child_count() > 0 && node.child(0)->text() == "(") {
                    return Expression(node.child(1).value());
                } else if (
                    node.type_id() == ts::NODE_IDENTIFIER ||
                    node.type_id() == ts::NODE_FIELD_EXPRESSION ||
                    node.type_id() == ts::NODE_TABLE_INDEX) {
                    return VariableDeclarator(node);
                } else {
                    throw std::runtime_error("Not a prefix-node");
                }
            },
            [](PrefixTuple tuple) -> PrefixVariant {
                auto mod_var = std::get<PFX_VARIANT>(tuple);
                return std::visit(
                    overloaded{
                        [](Self self) -> PrefixVariant { return self; },
                        [](VariableDeclarator vd) -> PrefixVariant { return vd; },
                        [](FunctionCall fc) -> PrefixVariant { return fc; },
                        [](const std::shared_ptr<Expression>& exp) -> PrefixVariant {
                            return *exp;
                        }},
                    mod_var);
            }},
        this->content);
}
auto Prefix::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) { return convert_range(node.range()); },
            [](PrefixTuple tuple) { return std::get<RANGE>(tuple); }},
        this->content);
}
auto Prefix::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const PrefixTuple& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}

// Expression
Expression::Expression(ts::Node node) : content(node) {
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
Expression::Expression(const UnaryOperation& un, minilua::Range range)
    : content(make_tuple(un, range)) {}
Expression::Expression(const BinaryOperation& bin, minilua::Range range)
    : content(make_tuple(bin, range)) {}
Expression::Expression(const FunctionDefinition& fd, minilua::Range range)
    : content(make_tuple(fd, range)) {}
auto Expression::options() const -> ExpressionVariant {
    return std::visit(
        overloaded{
            [](ts::Node node) -> ExpressionVariant {
                if (node.type_id() == ts::NODE_SPREAD) {
                    return Spread();
                } else if (node.type_id() == ts::NODE_FUNCTION_DEFINITION) {
                    return FunctionDefinition(node);
                } else if (node.type_id() == ts::NODE_TABLE) {
                    return Table(node);
                } else if (node.type_id() == ts::NODE_BINARY_OPERATION) {
                    return BinaryOperation(node);
                } else if (node.type_id() == ts::NODE_UNARY_OPERATION) {
                    return UnaryOperation(node);
                } else if (node.type_id() == ts::NODE_STRING) {
                    return Literal(LiteralType::STRING, node.text(), node.range());
                } else if (node.type_id() == ts::NODE_NUMBER) {
                    return Literal(LiteralType::NUMBER, node.text(), node.range());
                } else if (node.type_id() == ts::NODE_NIL) {
                    return Literal(LiteralType::NIL, node.text(), node.range());
                } else if (node.type_id() == ts::NODE_TRUE) {
                    return Literal(LiteralType::TRUE, node.text(), node.range());
                } else if (node.type_id() == ts::NODE_FALSE) {
                    return Literal(LiteralType::FALSE, node.text(), node.range());
                } else if (node.type_id() == ts::NODE_IDENTIFIER) {
                    return Identifier(node);
                } else if (
                    node.type_id() == ts::NODE_SELF || node.type_id() == ts::NODE_FUNCTION_CALL ||
                    node.type_id() == ts::NODE_FIELD_EXPRESSION ||
                    node.type_id() == ts::NODE_TABLE_INDEX ||
                    (node.child(0).has_value() && node.child(0)->text() == "(")) {
                    return Prefix(node);
                } else {
                    throw std::runtime_error("Not an expression-node");
                }
            },
            [](ExpTup tuple) -> ExpressionVariant { return std::get<EXP_VARIANT>(tuple); }},
        this->content);
}
auto Expression::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](ExpTup tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto Expression::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const ExpTup& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}

// Statement
Statement::Statement(ts::Node node) : content(node) {
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
Statement::Statement(const IfStatement& if_statement, minilua::Range range)
    : content(make_tuple(if_statement, range)) {}
Statement::Statement(const FunctionCall& func_call, minilua::Range range)
    : content(make_tuple(func_call, range)) {}
Statement::Statement(const WhileStatement& while_statement, minilua::Range range)
    : content(make_tuple(while_statement, range)) {}
Statement::Statement(const VariableDeclaration& var_dec, minilua::Range range)
    : content(make_tuple(var_dec, range)) {}
Statement::Statement(const DoStatement& do_statement, minilua::Range range)
    : content(make_tuple(do_statement, range)) {}
auto Statement::options() const -> StatementVariant {
    return std::visit(
        overloaded{
            [](ts::Node node) -> StatementVariant {
                if (node.type_id() == ts::NODE_EXPRESSION) {
                    return Expression(node.named_child(0).value());
                } else if (
                    node.type_id() == ts::NODE_VARIABLE_DECLARATION ||
                    node.type_id() == ts::NODE_LOCAL_VARIABLE_DECLARATION) {
                    return VariableDeclaration(node);
                } else if (node.type_id() == ts::NODE_DO_STATEMENT) {
                    return DoStatement(node);
                } else if (node.type_id() == ts::NODE_IF_STATEMENT) {
                    return IfStatement(node);
                } else if (node.type_id() == ts::NODE_WHILE_STATEMENT) {
                    return WhileStatement(node);
                } else if (node.type_id() == ts::NODE_REPEAT_STATEMENT) {
                    return RepeatStatement(node);
                } else if (node.type_id() == ts::NODE_FOR_STATEMENT) {
                    return ForStatement(node);
                } else if (node.type_id() == ts::NODE_FOR_IN_STATEMENT) {
                    return ForInStatement(node);
                } else if (node.type_id() == ts::NODE_GOTO_STATEMENT) {
                    return GoTo(node);
                } else if (node.type_id() == ts::NODE_BREAK_STATEMENT) {
                    return Break();
                } else if (node.type_id() == ts::NODE_LABEL_STATEMENT) {
                    return Label(node);
                } else if (node.type_id() == ts::NODE_FUNCTION) {
                    return FunctionStatement(node);
                } else if (node.type_id() == ts::NODE_LOCAL_FUNCTION) {
                    return FunctionStatement(node);
                } else if (node.type_id() == ts::NODE_FUNCTION_CALL) {
                    return FunctionCall(node);
                } else {
                    throw std::runtime_error("Not a statement-node");
                }
            },
            [](StatementTuple tuple) -> StatementVariant { return std::get<STAT_VARIANT>(tuple); }},
        this->content);
}
auto Statement::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) { return convert_range(node.range()); },
            [](StatementTuple tuple) { return std::get<RANGE>(tuple); }},
        this->content);
}
auto Statement::raw() const -> std::optional<ts::Node> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<ts::Node> { return node; },
            [](const StatementTuple& /*tuple*/) -> std::optional<ts::Node> { return nullopt; }},
        this->content);
}

Literal::Literal(LiteralType type, std::string string, ts::Range range)
    : literal_content(std::move(string)), literal_type(type), literal_range(range){};
auto Literal::type() const -> LiteralType { return this->literal_type; }
auto Literal::content() const -> std::string { return this->literal_content; }
auto Literal::range() const -> minilua::Range { return convert_range(this->literal_range); }
} // namespace minilua::details::ast