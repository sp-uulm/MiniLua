#include "ast.hpp"
namespace minilua::details::ast {
/**
 * we have a for loop like this:
 *      for v = e1, e2, e3 do block end
 *
 * and are basically deriving this code from it:
 *
 * do                                                                       --do_statement
       local var, limit, step = tonumber(e1), tonumber(e2), tonumber(e3)    --statement_1
       if not (var and limit and step) then error() end                     --statement_2
       var = var - step                                                     --statement_3
       while true do                                                        --while_loop
         var = var + step                                                   --statement_w1
         if (step >= 0 and var > limit) or (step < 0 and var < limit) then  --statement_w2
           break
         end
         local v = var                                                      --statement_w3
         block                                                 --all the statements of the for loop
       end
   end
 * @return
 */
auto ForStatement::desugar() const -> DoStatement {
    GEN_CAUSE gen_cause = FOR_LOOP_DESUGAR;
    auto loop_exp = this->loop_expression();
    auto le_range = loop_exp.range();
    // 1st we generate Identifiers and their corresponding Expressions for the three loop variables
    auto id_var = Identifier("__begin", le_range, gen_cause);
    auto id_limit = Identifier("__limit", le_range, gen_cause);
    auto id_step = Identifier("__step", le_range, gen_cause);
    auto var_exp = Expression(id_var, gen_cause);
    auto limit_exp = Expression(id_limit, gen_cause);
    auto step_exp = Expression(id_step, gen_cause);
    // now we are on to statement_1
    std::vector<Expression> declarations;
    Prefix to_number_prefix = Prefix(
        VariableDeclarator(Identifier("tonumber", le_range, gen_cause), gen_cause), gen_cause);
    // we now generate the tonumber function calls
    FunctionCall start_to_number = FunctionCall(
        to_number_prefix, std::nullopt, std::vector<Expression>{loop_exp.start()}, le_range,
        gen_cause);
    auto start_exp = Expression(Prefix(start_to_number, gen_cause), gen_cause);
    FunctionCall end_to_number = FunctionCall(
        to_number_prefix, std::nullopt, std::vector<Expression>{loop_exp.end()}, le_range,
        gen_cause);
    auto end_exp = Expression(Prefix(end_to_number, gen_cause), gen_cause);
    // if there is no value given for the step size we default to 1
    if (loop_exp.step().has_value()) {
        FunctionCall step_to_number = FunctionCall(
            to_number_prefix, std::nullopt, std::vector<Expression>{loop_exp.step().value()},
            le_range, gen_cause);
        auto step_exp = Expression(Prefix(step_to_number, gen_cause), gen_cause);
        declarations = std::vector<Expression>{start_exp, end_exp, step_exp};
    } else {
        Literal lit = Literal(LiteralType::NUMBER, "1", loop_exp.range());
        declarations = std::vector<Expression>{start_exp, end_exp, Expression(lit, gen_cause)};
    }
    auto declarators = std::vector<VariableDeclarator>{
        VariableDeclarator(id_var, gen_cause), VariableDeclarator(id_limit, gen_cause),
        VariableDeclarator(id_step, gen_cause)};
    auto statement_1 = VariableDeclaration(true, declarators, declarations, le_range, gen_cause);
    // on to statement_2
    // first we generate the body of the if statement which is just a call of the error function
    auto error_call = Statement(
        FunctionCall(
            Prefix(
                VariableDeclarator(Identifier("error", le_range, gen_cause), gen_cause), gen_cause),
            std::nullopt, std::vector<Expression>(), le_range, gen_cause),
        gen_cause);
    auto if_body = Body(std::vector<Statement>{error_call}, std::nullopt);
    // then we generate the condition
    auto if_condition = UnaryOperation(
        UnOpEnum::NOT,
        Expression(
            BinaryOperation(
                var_exp, BinOpEnum::AND,
                Expression(
                    BinaryOperation(limit_exp, BinOpEnum::AND, step_exp, le_range, gen_cause),
                    gen_cause),
                le_range, gen_cause),
            gen_cause),
        le_range, gen_cause);
    // and put it all together into an if statement
    auto statement_2 =
        IfStatement(Expression(if_condition, gen_cause), if_body, le_range, gen_cause);
    // statement_3 is pretty short and probably self explanatory
    VariableDeclaration statement_3 = VariableDeclaration(
        false, std::vector<VariableDeclarator>{VariableDeclarator(id_var, gen_cause)},
        std::vector<Expression>{Expression(
            BinaryOperation(var_exp, BinOpEnum::SUB, step_exp, le_range, gen_cause), gen_cause)},
        le_range, gen_cause);
    // we now generate the statements inside the while_loop
    VariableDeclaration statement_w1 = VariableDeclaration(
        false, std::vector<VariableDeclarator>{VariableDeclarator(id_var, gen_cause)},
        std::vector<Expression>{Expression(
            BinaryOperation(var_exp, BinOpEnum::ADD, step_exp, le_range, gen_cause), gen_cause)},
        le_range, gen_cause);
    // statement_w2 is a little more complex because of the long condition "(step >= 0 and var >
    // limit) or (step < 0 and var < limit)"
    auto zero_exp = Expression(Literal(LiteralType::NUMBER, "0", le_range), gen_cause);
    // we first generate the left part "(step >= 0 and var > limit)"
    auto if_cond_left = Expression(
        BinaryOperation(
            Expression(
                BinaryOperation(step_exp, BinOpEnum::GEQ, zero_exp, le_range, gen_cause),
                gen_cause),
            BinOpEnum::AND,
            Expression(
                BinaryOperation(var_exp, BinOpEnum::GT, limit_exp, le_range, gen_cause), gen_cause),
            le_range, gen_cause),
        gen_cause);
    // then the right part "(step < 0 and var < limit)"
    auto if_cond_right = Expression(
        BinaryOperation(
            Expression(
                BinaryOperation(step_exp, BinOpEnum::LT, zero_exp, le_range, gen_cause), gen_cause),
            BinOpEnum::AND,
            Expression(
                BinaryOperation(var_exp, BinOpEnum::LT, limit_exp, le_range, gen_cause), gen_cause),
            le_range, gen_cause),
        gen_cause);
    // and we then put those two parts together
    auto complete_if_cond = Expression(
        BinaryOperation(if_cond_left, BinOpEnum::OR, if_cond_right, le_range, gen_cause),
        gen_cause);
    // then we generate the body of the if statement alias statement_w2
    auto statement_w2_body =
        Body(std::vector<Statement>{Statement(Break(), le_range, gen_cause)}, std::nullopt);
    auto statement_w2 = IfStatement(complete_if_cond, statement_w2_body, le_range, gen_cause);
    // statement_w3 is just a simple assignment
    VariableDeclaration statement_w3 = VariableDeclaration(
        true, std::vector<VariableDeclarator>{VariableDeclarator(loop_exp.variable(), gen_cause)},
        std::vector<Expression>{var_exp}, le_range, gen_cause);
    // let's put the statements of the while_loop body together now
    auto while_statements = std::vector<Statement>{
        Statement(statement_w1, gen_cause), Statement(statement_w2, gen_cause),
        Statement(statement_w3, gen_cause)};
    auto for_statements = this->body().statements();
    // we now have to append the statements of the initial for loop
    while_statements.insert(while_statements.end(), for_statements.begin(), for_statements.end());
    Body while_body = Body(while_statements, this->body().return_statement());
    // and now we can construct the while_loop
    auto while_loop = WhileStatement(
        Expression(Literal(LiteralType::TRUE, "true", le_range), gen_cause), while_body, le_range,
        gen_cause);
    // lastly we have to encapsulate statements 1-3 and the while loop in a do_statement
    Body do_body = Body(
        std::vector<Statement>{
            Statement(statement_1, gen_cause), Statement(statement_2, gen_cause),
            Statement(statement_3, gen_cause), Statement(while_loop, gen_cause)},
        std::nullopt);
    return DoStatement(do_body, this->range(), gen_cause);
}
/**
 * This function nests a vector of identifiers into FieldExpressions
 * @param id_vec
 * @param range
 * @param gen_cause
 * @return
 */
auto identifier_vector_to_variable_declarator(
    std::vector<Identifier> id_vec, const minilua::Range& range, GEN_CAUSE gen_cause)
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
            iterator++;
        }
        return VariableDeclarator(current_fe, gen_cause);
    }
}
/**
 *
 * @return
 */
auto FunctionStatement::desugar() const -> VariableDeclaration {
    GEN_CAUSE gen_cause = FUNCTION_STATEMENT_DESUGAR;
    auto id_vector = this->name().identifier();
    // we generate a variable_declarator from the function_name
    auto method_optional = this->name().method();
    if (method_optional.has_value()) {
        id_vector.push_back(method_optional.value());
    }
    auto declarator =
        identifier_vector_to_variable_declarator(id_vector, this->name().range(), gen_cause);
    if (method_optional.has_value()) {
        auto ids = this->parameters().params();
        // we have to adjust the parameters here and add a self
        ids.insert(ids.begin(), Identifier("self", method_optional.value().range(), gen_cause));
        Parameters params =
            Parameters(ids, this->parameters().spread(), this->parameters().range(), gen_cause);
        FunctionDefinition fd = FunctionDefinition(params, this->body(), this->range(), gen_cause);
        return VariableDeclaration(
            this->local(), std::vector<VariableDeclarator>{declarator},
            std::vector<Expression>{Expression(fd, gen_cause)}, this->range(), gen_cause);
    } else {
        FunctionDefinition fd =
            FunctionDefinition(this->parameters(), this->body(), this->range(), gen_cause);
        return VariableDeclaration(
            this->local(), std::vector<VariableDeclarator>{declarator},
            std::vector<Expression>{Expression(fd, gen_cause)}, this->range(), gen_cause);
    }
}
/**
 * we have a for loop like this:
 *      for var_1, ···, var_n in explist do block end
 *
 * and are basically deriving this code from it:
 *
 *    do                                            --do_statement
       local f, s, var = explist                    --statement_1
       while true do                                --while_statement
         local var_1, ···, var_n = f(s, var)        --statement_w1
         if var_1 == nil then break end             --statement_w2
         var = var_1                                --statement_w3
         block                                      --all statements of the for loop
       end
     end
 *
 * @return
 */
auto ForInStatement::desugar() const -> DoStatement {
    GEN_CAUSE gen_cause = GEN_CAUSE::FOR_IN_LOOP_DESUGAR;
    InLoopExpression le = this->loop_expression();
    Identifier f = Identifier("__func", le.range(), gen_cause);
    Identifier s = Identifier("__s", le.range(), gen_cause);
    Identifier var = Identifier("__var", le.range(), gen_cause);
    // statement_1 is just a simple variable declaration
    VariableDeclaration statement_1 = VariableDeclaration(
        true,
        std::vector<VariableDeclarator>{
            VariableDeclarator(f, gen_cause), VariableDeclarator(s, gen_cause),
            VariableDeclarator(var, gen_cause)},
        le.loop_exps(), le.range(), gen_cause);
    // we now generate the function call that is the right side of the assignment in statement_w1
    FunctionCall fc = FunctionCall(
        Prefix(VariableDeclarator(f, gen_cause), gen_cause), std::nullopt,
        std::vector<Expression>{Expression(s, gen_cause), Expression(var, gen_cause)}, le.range(),
        gen_cause);
    // we now get our loop variables which are the left side of the assignment in statement_w1
    std::vector<VariableDeclarator> declarators;
    auto ids = loop_expression().loop_vars();
    std::transform(
        ids.begin(), ids.end(), std::back_inserter(declarators),
        [gen_cause](const Identifier& id) { return VariableDeclarator(id, gen_cause); });
    // now we can generate out next statement
    VariableDeclaration statement_w1 = VariableDeclaration(
        true, declarators, std::vector<Expression>{Expression(Prefix(fc, gen_cause), gen_cause)},
        le.range(), gen_cause);
    // now we generate the if statement alias statement_w2
    Body if_body =
        Body(std::vector<Statement>{Statement(Break(), le.range(), gen_cause)}, std::nullopt);
    Expression if_condition = Expression(
        BinaryOperation(
            Expression(ids[0], gen_cause), BinOpEnum::EQ,
            Expression(Literal(LiteralType::NIL, "nil", le.range()), gen_cause), le.range(),
            gen_cause),
        gen_cause);
    IfStatement statement_w2 = IfStatement(if_condition, if_body, le.range(), gen_cause);
    // we generate the short variable declaration alias statement_w3
    VariableDeclaration statement_w3 = VariableDeclaration(
        false, std::vector<VariableDeclarator>{VariableDeclarator(var, gen_cause)},
        std::vector<Expression>{Expression(ids[0], gen_cause)}, le.range(), gen_cause);
    // we put the statements in the while loop together in a vector
    auto while_statements = std::vector<Statement>{
        Statement(statement_w1, gen_cause), Statement(statement_w2, gen_cause),
        Statement(statement_w3, gen_cause)};
    // and after that we append the statements inside the initial for loop
    auto for_statements = this->body().statements();
    while_statements.insert(while_statements.end(), for_statements.begin(), for_statements.end());
    auto while_body = Body(while_statements, this->body().return_statement());
    // now we can construct the while_loop
    auto while_loop = WhileStatement(
        Expression(Literal(LiteralType::TRUE, "true", le.range()), gen_cause), while_body,
        le.range(), gen_cause);
    // lastly we have to encapsulate statement_1 and the while loop in a do_statement
    auto do_body = Body(
        std::vector<Statement>{Statement(statement_1, gen_cause), Statement(while_loop, gen_cause)},
        std::nullopt);
    return DoStatement(do_body, this->range(), gen_cause);
}

} // namespace minilua::details::ast