#include "ast.hpp"
namespace minilua::details::ast {
auto ForStatement::desugar() const -> DoStatement {
    GEN_CAUSE gen_cause = FOR_LOOP_DESUGAR;
    auto loop_exp = this->loop_expression();
    auto le_range = loop_exp.range();
    auto id_var = Identifier("__begin", le_range, gen_cause);
    auto id_limit = Identifier("__limit", le_range, gen_cause);
    auto id_step = Identifier("__step", le_range, gen_cause);

    auto var_exp = Expression(id_var);
    auto limit_exp = Expression(id_limit);
    auto step_exp = Expression(id_step);
    std::vector<Expression> declarations;
    Prefix to_number_prefix = Prefix(
        VariableDeclarator(Identifier("to_number", le_range, gen_cause), gen_cause), gen_cause);
    FunctionCall to_number = FunctionCall(
        to_number_prefix, std::nullopt,
        std::vector<Expression>{
            loop_exp.start(), Expression(Literal(LiteralType::NUMBER, "10", le_range))},
        le_range, gen_cause);
    auto start_exp = Expression(Prefix(to_number, gen_cause));
    if (loop_exp.step().has_value()) {
        declarations =
            std::vector<Expression>{loop_exp.start(), loop_exp.end(), loop_exp.step().value()};
    } else {
        Literal lit = Literal(LiteralType::NUMBER, "1", loop_exp.range());
        declarations = std::vector<Expression>{loop_exp.start(), loop_exp.end(), Expression(lit)};
    }
    auto declarators = std::vector<VariableDeclarator>{
        VariableDeclarator(id_var, gen_cause), VariableDeclarator(id_limit, gen_cause),
        VariableDeclarator(id_step, gen_cause)};
    /*1*/ auto var_dec = VariableDeclaration(true, declarators, declarations, le_range, gen_cause);

    auto error_call = Statement(FunctionCall(
        Prefix(VariableDeclarator(Identifier("error", le_range, gen_cause), gen_cause), gen_cause),
        std::nullopt, std::vector<Expression>(), le_range, gen_cause));
    auto if_body = Body(std::vector<Statement>{error_call}, std::nullopt);
    auto if_condition = UnaryOperation(
        UnOpEnum::NOT,
        Expression(BinaryOperation(
            var_exp, BinOpEnum::AND,
            Expression(BinaryOperation(limit_exp, BinOpEnum::AND, step_exp, le_range, gen_cause)),
            le_range, gen_cause)),
        le_range, gen_cause);

    /*2*/ auto if_statement = IfStatement(Expression(if_condition), if_body, le_range, gen_cause);

    /*3*/ VariableDeclaration vd1 = VariableDeclaration(
        false, std::vector<VariableDeclarator>{VariableDeclarator(id_var, gen_cause)},
        std::vector<Expression>{
            Expression(BinaryOperation(var_exp, BinOpEnum::SUB, step_exp, le_range, gen_cause))},
        le_range, gen_cause);

    /*W1*/ VariableDeclaration vd2 = VariableDeclaration(
        false, std::vector<VariableDeclarator>{VariableDeclarator(id_var, gen_cause)},
        std::vector<Expression>{
            Expression(BinaryOperation(var_exp, BinOpEnum::ADD, step_exp, le_range, gen_cause))},
        le_range, gen_cause);
    auto zero_exp = Expression(Literal(LiteralType::NUMBER, "0", le_range));
    auto if_cond_left = Expression(BinaryOperation(
        Expression(BinaryOperation(step_exp, BinOpEnum::GEQ, zero_exp, le_range, gen_cause)),
        BinOpEnum::AND,
        Expression(BinaryOperation(var_exp, BinOpEnum::GT, limit_exp, le_range, gen_cause)),
        le_range, gen_cause));
    auto if_cond_right = Expression(BinaryOperation(
        Expression(BinaryOperation(step_exp, BinOpEnum::LT, zero_exp, le_range, gen_cause)),
        BinOpEnum::AND,
        Expression(BinaryOperation(var_exp, BinOpEnum::LT, limit_exp, le_range, gen_cause)),
        le_range, gen_cause));
    auto while_if_body = Body(std::vector<Statement>{Statement(Break(), le_range)}, std::nullopt);
    /*W2*/ auto if_statement2 = IfStatement(
        Expression(
            BinaryOperation(if_cond_left, BinOpEnum::OR, if_cond_right, le_range, gen_cause)),
        while_if_body, le_range, gen_cause);
    /*W3*/ VariableDeclaration vd3 = VariableDeclaration(
        true, std::vector<VariableDeclarator>{VariableDeclarator(loop_exp.variable(), gen_cause)},
        std::vector<Expression>{var_exp}, le_range, gen_cause);
    auto for_statements = this->body().statements();
    auto while_statements =
        std::vector<Statement>{Statement(vd2), Statement(if_statement2), Statement(vd3)};
    while_statements.insert(while_statements.end(), for_statements.begin(), for_statements.end());
    Body while_body = Body(while_statements, this->body().return_statement());

    /*4*/ auto while_loop = WhileStatement(
        Expression(Literal(LiteralType::TRUE, "true", le_range)), while_body, le_range, gen_cause);
    Body do_body = Body(
        std::vector<Statement>{
            Statement(var_dec), Statement(if_statement), Statement(vd1), Statement(while_loop)},
        std::nullopt);
    return DoStatement(do_body, this->range(), gen_cause);
}
auto identifier_vector_to_variable_declarator(
    std::vector<Identifier> id_vec, minilua::Range range, GEN_CAUSE gen_cause)
    -> VariableDeclarator {
    if (id_vec.empty()) {
        throw std::runtime_error("no empty vectors allowed in this method");
    } else if (id_vec.size() == 1) {
        return VariableDeclarator(id_vec[0], gen_cause);
    } else {
        auto iterator = id_vec.begin() + 2;
        auto current_fe = FieldExpression(
            Prefix(VariableDeclarator(id_vec[0], gen_cause), gen_cause), id_vec[1], range,
            gen_cause);
        while (iterator != id_vec.end()) {
            current_fe = FieldExpression(
                Prefix(VariableDeclarator(current_fe, gen_cause), gen_cause), *iterator, range,
                gen_cause);
        }
        return VariableDeclarator(current_fe, gen_cause);
    }
};
auto FunctionStatement::desugar() const -> VariableDeclaration {
    GEN_CAUSE gen_cause = FUNCTION_STATEMENT_DESUGAR;
    FunctionDefinition fd =
        FunctionDefinition(this->parameters(), this->body(), this->range(), gen_cause);
    auto id_vector = this->name().identifier();
    auto method_optional = this->name().method();
    if (method_optional.has_value()) {
        id_vector.push_back(method_optional.value());
    }
    auto declarator =
        identifier_vector_to_variable_declarator(id_vector, this->name().range(), gen_cause);
    if (method_optional.has_value()) {
        auto ids = this->parameters().params();
        ids.insert(ids.begin(), Identifier("self", method_optional.value().range(), gen_cause));
        Parameters params =
            Parameters(ids, this->parameters().spread(), this->parameters().range(), gen_cause);
        FunctionDefinition fd = FunctionDefinition(params, this->body(), this->range(), gen_cause);
        return VariableDeclaration(
            this->local(), std::vector<VariableDeclarator>{declarator},
            std::vector<Expression>{Expression(fd)}, this->range(), gen_cause);
    } else {
        FunctionDefinition fd =
            FunctionDefinition(this->parameters(), this->body(), this->range(), gen_cause);
        return VariableDeclaration(
            this->local(), std::vector<VariableDeclarator>{declarator},
            std::vector<Expression>{Expression(fd)}, this->range(), gen_cause);
    }
}

auto ForInStatement::desugar() const -> DoStatement {
    GEN_CAUSE gen_cause = GEN_CAUSE::FOR_IN_LOOP_DESUGAR;
    InLoopExpression le = this->loop_expression();
    Identifier f = Identifier("__func", le.range(), gen_cause);
    Identifier s = Identifier("__table", le.range(), gen_cause);
    Identifier var = Identifier("__key", le.range(), gen_cause);
    /*1*/ VariableDeclaration vd_1 = VariableDeclaration(
        true,
        std::vector<VariableDeclarator>{
            VariableDeclarator(f, gen_cause), VariableDeclarator(s, gen_cause),
            VariableDeclarator(var, gen_cause)},
        le.loop_exps(), le.range(), gen_cause);
    FunctionCall fc = FunctionCall(
        Prefix(VariableDeclarator(f, gen_cause), gen_cause), std::nullopt,
        std::vector<Expression>{Expression(s), Expression(var)}, le.range(), gen_cause);
    std::vector<VariableDeclarator> declarators;
    auto ids = loop_expression().loop_vars();
    std::transform(
        ids.begin(), ids.end(), std::back_inserter(declarators),
        [gen_cause](Identifier id) { return VariableDeclarator(id, gen_cause); });
    /*W1*/ VariableDeclaration vd_2 = VariableDeclaration(
        true, declarators, std::vector<Expression>{Expression(Prefix(fc, gen_cause))}, le.range(),
        gen_cause);
    Body if_body = Body(std::vector<Statement>{Statement(Break(), le.range())}, std::nullopt);
    Expression if_condition = Expression(BinaryOperation(
        Expression(ids[0]), BinOpEnum::EQ, Expression(Literal(LiteralType::NIL, "nil", le.range())),
        le.range(), gen_cause));
    /*W2*/ IfStatement if_stat = IfStatement(if_condition, if_body, le.range(), gen_cause);
    /*W3*/ VariableDeclaration vd_3 = VariableDeclaration(
        false, std::vector<VariableDeclarator>{VariableDeclarator(var, gen_cause)},
        std::vector<Expression>{Expression(ids[0])}, le.range(), gen_cause);
    auto for_statements = this->body().statements();
    auto while_statements =
        std::vector<Statement>{Statement(vd_2), Statement(if_stat), Statement(vd_3)};
    while_statements.insert(while_statements.end(), for_statements.begin(), for_statements.end());
    auto while_body = Body(while_statements, this->body().return_statement());
    auto while_loop = WhileStatement(
        Expression(Literal(LiteralType::TRUE, "true", le.range())), while_body, le.range(),
        gen_cause);
    auto do_body =
        Body(std::vector<Statement>{Statement(vd_1), Statement(while_loop)}, std::nullopt);
    return DoStatement(do_body, this->range(), gen_cause);
}

} // namespace minilua::details::ast