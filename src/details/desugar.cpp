#include "ast.hpp"
namespace minilua::details::ast {
auto ForStatement::desugar() const -> DoStatement {
    auto loop_exp = this->loop_expression();
    auto le_range = loop_exp.range();
    auto id_var = Identifier("__begin", le_range);
    auto id_limit = Identifier("limit", le_range);
    auto id_step = Identifier("step", le_range);

    auto var_exp = Expression(id_var, le_range);
    auto limit_exp = Expression(id_limit, le_range);
    auto step_exp = Expression(id_step, le_range);
    std::vector<Expression> declarations;
    if (loop_exp.step().has_value()) {
        declarations =
            std::vector<Expression>{loop_exp.start(), loop_exp.end(), loop_exp.step().value()};
    } else {
        Literal lit = Literal(LiteralType::NUMBER, "1", loop_exp.range());
        declarations = std::vector<Expression>{
            loop_exp.start(), loop_exp.end(), Expression(lit, loop_exp.range())};
    }
    auto declarators = std::vector<VariableDeclarator>{
        VariableDeclarator(id_var), VariableDeclarator(id_limit), VariableDeclarator(id_step)};
    /*1*/ auto var_dec = VariableDeclaration(true, declarators, declarations, le_range);

    auto error_call = Statement(
        FunctionCall(
            Prefix(VariableDeclarator(Identifier("error", le_range)), le_range), nullopt,
            std::vector<Expression>(), le_range),
        le_range);
    auto if_body = Body(std::vector<Statement>{error_call}, nullopt);
    auto if_condition = UnaryOperation(
        UnOpEnum::NOT,
        Expression(
            BinaryOperation(
                var_exp, BinOpEnum::AND,
                Expression(
                    BinaryOperation(limit_exp, BinOpEnum::AND, step_exp, le_range), le_range),
                le_range),
            le_range),
        le_range);

    /*2*/ auto if_statement = IfStatement(Expression(if_condition, le_range), if_body, le_range);

    /*3*/ VariableDeclaration vd1 = VariableDeclaration(
        false, std::vector<VariableDeclarator>{VariableDeclarator(id_var)},
        std::vector<Expression>{
            Expression(BinaryOperation(var_exp, BinOpEnum::SUB, step_exp, le_range), le_range)},
        le_range);

    /*W1*/ VariableDeclaration vd2 = VariableDeclaration(
        false, std::vector<VariableDeclarator>{VariableDeclarator(id_var)},
        std::vector<Expression>{
            Expression(BinaryOperation(var_exp, BinOpEnum::ADD, step_exp, le_range), le_range)},
        le_range);
    auto zero_exp = Expression(Literal(LiteralType::NUMBER, "0", le_range), le_range);
    auto if_cond_left = Expression(
        BinaryOperation(
            Expression(BinaryOperation(step_exp, BinOpEnum::GEQ, zero_exp, le_range), le_range),
            BinOpEnum::AND,
            Expression(BinaryOperation(var_exp, BinOpEnum::GT, limit_exp, le_range), le_range),
            le_range),
        le_range);
    auto if_cond_right = Expression(
        BinaryOperation(
            Expression(BinaryOperation(step_exp, BinOpEnum::LT, zero_exp, le_range), le_range),
            BinOpEnum::AND,
            Expression(BinaryOperation(var_exp, BinOpEnum::LT, limit_exp, le_range), le_range),
            le_range),
        le_range);
    auto while_if_body = Body(std::vector<Statement>{Statement(Break(), le_range)}, nullopt);
    /*W2*/ auto if_statement2 = IfStatement(
        Expression(BinaryOperation(if_cond_left, BinOpEnum::OR, if_cond_right, le_range), le_range),
        while_if_body, le_range);
    /*W3*/ VariableDeclaration vd3 = VariableDeclaration(
        true, std::vector<VariableDeclarator>{VariableDeclarator(loop_exp.variable())},
        std::vector<Expression>{var_exp}, le_range);
    auto for_statements = this->body().statements();
    auto while_statements = std::vector<Statement>{
        Statement(vd2, le_range), Statement(if_statement2, le_range), Statement(vd3, le_range)};
    while_statements.insert(while_statements.end(), for_statements.begin(), for_statements.end());
    Body while_body = Body(while_statements, this->body().return_statement());

    /*4*/ auto while_loop = WhileStatement(
        Expression(Literal(LiteralType::TRUE, "true", le_range), le_range), while_body, le_range);
    Body do_body = Body(
        std::vector<Statement>{
            Statement(var_dec, le_range), Statement(if_statement, le_range),
            Statement(vd1, le_range), Statement(while_loop, le_range)},
        nullopt);
    return DoStatement(do_body, this->range());
}
auto identifier_vector_to_variable_declarator(std::vector<Identifier> id_vec, minilua::Range range)
    -> VariableDeclarator {
    if (id_vec.empty()) {
        throw std::runtime_error("no empty vectors allowed in this method");
    } else if (id_vec.size() == 1) {
        return VariableDeclarator(id_vec[0]);
    } else {
        auto iterator = id_vec.begin() + 2;
        auto current_fe =
            FieldExpression(Prefix(VariableDeclarator(id_vec[0]), range), id_vec[1], range);
        while (iterator != id_vec.end()) {
            current_fe =
                FieldExpression(Prefix(VariableDeclarator(current_fe), range), *iterator, range);
        }
        return VariableDeclarator(current_fe);
    }
};
auto FunctionStatement::desugar() const -> VariableDeclaration {
    FunctionDefinition fd = FunctionDefinition(this->parameters(), this->body(), this->range());
    auto id_vector = this->name().identifier();
    auto method_optional = this->name().method();
    if (method_optional.has_value()) {
        id_vector.push_back(method_optional.value());
    }
    auto declarator = identifier_vector_to_variable_declarator(id_vector, this->name().range());
    if (method_optional.has_value()) {
        auto ids = this->parameters().params();
        ids.insert(ids.begin(), Identifier("self", method_optional.value().range()));
        Parameters params =
            Parameters(ids, this->parameters().spread(), this->parameters().range());
        FunctionDefinition fd = FunctionDefinition(params, this->body(), this->range());
        return VariableDeclaration(
            this->local(), std::vector<VariableDeclarator>{declarator},
            std::vector<Expression>{Expression(fd, this->range())}, this->range());
    } else {
        FunctionDefinition fd = FunctionDefinition(this->parameters(), this->body(), this->range());
        return VariableDeclaration(
            this->local(), std::vector<VariableDeclarator>{declarator},
            std::vector<Expression>{Expression(fd, this->range())}, this->range());
    }
}

auto ForInStatement::desugar() const -> DoStatement {
    InLoopExpression le = this->loop_expression();
    Identifier f = Identifier("__func", le.range());
    Identifier s = Identifier("__table", le.range());
    Identifier var = Identifier("__key", le.range());
    /*1*/ VariableDeclaration vd_1 = VariableDeclaration(
        true,
        std::vector<VariableDeclarator>{
            VariableDeclarator(f), VariableDeclarator(s), VariableDeclarator(var)},
        le.loop_exps(), le.range());
    FunctionCall fc = FunctionCall(
        Prefix(VariableDeclarator(f), f.range()), nullopt,
        std::vector<Expression>{Expression(s, s.range()), Expression(var, var.range())},
        le.range());
    std::vector<VariableDeclarator> declarators;
    auto ids = loop_expression().loop_vars();
    std::transform(ids.begin(), ids.end(), std::back_inserter(declarators), [](Identifier id) {
        return VariableDeclarator(id);
    });
    /*W1*/ VariableDeclaration vd_2 = VariableDeclaration(
        true, declarators, std::vector<Expression>{Expression(Prefix(fc))}, le.range());
    Body if_body = Body(std::vector<Statement>{Statement(Break(), le.range())}, nullopt);
    Expression if_condition = Expression(
        BinaryOperation(
            Expression(ids[0], ids[0].range()), BinOpEnum::EQ,
            Expression(Literal(LiteralType::NIL, "nil", le.range()), le.range()), le.range()),
        le.range());
    /*W2*/ IfStatement if_stat = IfStatement(if_condition, if_body, le.range());
    /*W3*/ VariableDeclaration vd_3 = VariableDeclaration(
        false, std::vector<VariableDeclarator>{VariableDeclarator(var)},
        std::vector<Expression>{Expression(ids[0], ids[0].range())}, le.range());
    auto for_statements = this->body().statements();
    auto while_statements = std::vector<Statement>{
        Statement(vd_2, vd_2.range()), Statement(if_stat, if_stat.range()),
        Statement(vd_3, vd_3.range())};
    while_statements.insert(while_statements.end(), for_statements.begin(), for_statements.end());
    auto while_body = Body(while_statements, this->body().return_statement());
    auto while_loop = WhileStatement(
        Expression(Literal(LiteralType::TRUE, "true", le.range()), le.range()), while_body,
        le.range());
    auto do_body = Body(
        std::vector<Statement>{
            Statement(vd_1, vd_1.range()), Statement(while_loop, while_loop.range())},
        nullopt);
    return DoStatement(do_body, this->range());
}

} // namespace minilua::details::ast