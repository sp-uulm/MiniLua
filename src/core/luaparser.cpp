#include "luaparser.h"

LuaParser::LuaParser() {

}

auto LuaParser::parse(const string program, PerformanceStatistics& ps) -> parse_result_t<LuaChunk> {
    auto tokenize_start = chrono::steady_clock::now();
    tokens = tokenize(begin(program), end(program));
    auto tokenize_end = chrono::steady_clock::now();
//    for (const auto& token : tokens)
//        cout << token << endl;

    token_it_t begin_tok = tokens.cbegin();
    token_it_t end_tok = tokens.cend()-1; //end_token is not part of the program
    auto parse_result = parse_chunk(begin_tok, end_tok);

    auto parse_end = chrono::steady_clock::now();

    ps.tokenize = chrono::duration_cast<chrono::microseconds>(tokenize_end - tokenize_start);
    ps.parse = chrono::duration_cast<chrono::microseconds>(parse_end - tokenize_end);

    return parse_result;
}

auto LuaParser::tokenize(string::const_iterator first, string::const_iterator last) -> token_list_t {
    string ws = "";

    token_list_t result_tokens;
    bool tokenize_result = bs::lex::tokenize(first, last, lua_lexer, [&, start = first](const auto& token) {
        if (token.id() == WS) {
            ws = string(token.value().begin(), token.value().end());
            return true;
        }

        string match = string(token.value().begin(), token.value().end());
        long pos = token.value().begin() - start;
        long length = static_cast<long>(match.size());
        result_tokens.push_back(LuaToken{static_cast<LuaToken::Type>(token.id()), match, pos, length, ws});
        ws = "";
        return true;
    });

    if (!tokenize_result) {
        cerr << "tokenizing failed" << endl;
    }

    LuaToken end_token {LuaToken::Type::NONE, "", -1, 0, ws};
    result_tokens.push_back(end_token);

    return result_tokens;
}

auto LuaParser::parse_chunk(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaChunk> {
    //cout << "chunk" << endl;
    // chunk ::= {stat [`;´]} [laststat [`;´]]

    LuaChunk result = make_shared<_LuaChunk>();

    while (begin != end &&
           begin->type != LuaToken::Type::RETURN &&
           begin->type != LuaToken::Type::BREAK &&
           begin->type != LuaToken::Type::END &&
           begin->type != LuaToken::Type::ELSE &&
           begin->type != LuaToken::Type::ELSEIF &&
           begin->type != LuaToken::Type::UNTIL) {
        auto ast = parse_stat(begin, end);
        if (holds_alternative<string>(ast)) {
            return "chunk -> " + get<string>(ast);
        } else {
            result->statements.push_back(get<LuaStmt>(ast));
        }

        if (begin->type == LuaToken::Type::SEM)
            begin++;
    }

    if (begin != end && (begin->type == LuaToken::Type::RETURN || begin->type == LuaToken::Type::BREAK)) {
        auto ast = parse_laststat(begin, end);
        if (holds_alternative<string>(ast)) {
            return "chunk -> " + get<string>(ast);
        } else {
            result->statements.push_back(get<LuaStmt>(ast));
        }

        if (begin->type == LuaToken::Type::SEM)
            begin++;
    }

    return move(result);
}

auto LuaParser::parse_block(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaChunk> {
    //cout << "block" << endl;
    return parse_chunk(begin, end);
}

auto LuaParser::parse_stat(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaStmt> {
    //cout << "stat" << endl;
    /* stat ::=     varlist `=´ explist |
                    functioncall |
                    do block end |
                    while exp do block end |
                    repeat block until exp |
                    if exp then block {elseif exp then block} [else block] end |
                    for Name `=´ exp `,´ exp [`,´ exp] do block end |
                    for namelist in explist do block end |
                    function funcname funcbody |
                    local function Name funcbody |
                    local namelist [`=´ explist] */

    if (begin == end)
        return "stat: unexpected end";

    auto stat_begin = begin;

    switch (begin->type) {
    case LuaToken::Type::NAME: // varlist, functioncall
    {
        token_it_t old_begin = begin;
        if (auto prefix = parse_prefixexp(begin, end); holds_alternative<LuaExp>(prefix)) {
            auto callexp = get<LuaExp>(prefix);
            if (auto call = dynamic_pointer_cast<_LuaFunctioncall>(callexp); call) {
                copy(stat_begin, begin, back_inserter(call->tokens));
                return static_pointer_cast<_LuaStmt>(call);
            }
        }
        begin = old_begin;

        auto assign = make_shared<_LuaAssignment>();
        auto varlist = parse_varlist(begin, end);
        if (holds_alternative<string>(varlist)) {
            return "stat (assignment) -> " + get<string>(varlist);
        }
        assign->varlist = get<LuaExplist>(varlist);

        if (begin++->type != LuaToken::Type::ASSIGN) {
            return "stat (assignment): '=' expected";
        }

        auto explist = parse_explist(begin, end);
        if (holds_alternative<string>(explist)) {
            return "stat (assignment) -> " + get<string>(explist);
        }
        assign->explist = get<LuaExplist>(explist);

        copy(stat_begin, begin, back_inserter(assign->tokens));
        return move(assign);
    }
    case LuaToken::Type::DO:
        return "unimplemented";
    case LuaToken::Type::WHILE:
    {
        begin++; //while
        LuaLoopStmt while_stmt = make_shared<_LuaLoopStmt>();
        while_stmt->head_controlled = true;

        if (auto exp = parse_exp(begin, end); holds_alternative<string>(exp)) {
            return "stat (while) -> " + get<string>(exp);
        } else {
            while_stmt->end = get<LuaExp>(exp);
        }

        if (begin++->type != LuaToken::Type::DO) {
            return "stat (while): 'do' expected";
        }

        if (auto block = parse_block(begin, end); holds_alternative<string>(block)) {
            return "stat (while) -> " + get<string>(block);
        } else {
            while_stmt->body = get<LuaChunk>(block);
        }

        if (begin++->type != LuaToken::Type::END) {
            return "stat (while): 'end' expected";
        }

        copy(stat_begin, begin, back_inserter(while_stmt->tokens));
        return move(while_stmt);
    }
    case LuaToken::Type::REPEAT:
    {
        begin++; //repeat
        LuaLoopStmt repeat_stmt = make_shared<_LuaLoopStmt>();
        repeat_stmt->head_controlled = false;

        if (auto block = parse_block(begin, end); holds_alternative<string>(block)) {
            return "stat (repeat) -> " + get<string>(block);
        } else {
            repeat_stmt->body = get<LuaChunk>(block);
        }

        if (begin++->type != LuaToken::Type::UNTIL) {
            return "stat (repeat): 'until' expected";
        }

        if (auto exp = parse_exp(begin, end); holds_alternative<string>(exp)) {
            return "stat (repeat) -> " + get<string>(exp);
        } else {
            repeat_stmt->end = _LuaUnop::Not(get<LuaExp>(exp));
        }

        copy(stat_begin, begin, back_inserter(repeat_stmt->tokens));
        return move(repeat_stmt);
    }
    case LuaToken::Type::IF:
    {
        begin++;

        LuaIfStmt if_stmt = make_shared<_LuaIfStmt>();
        if_stmt->branches.emplace_back();

        if (auto exp = parse_exp(begin, end); holds_alternative<string>(exp)) {
            return "stat (if) -> " + get<string>(exp);
        } else {
            if_stmt->branches.back().first = get<LuaExp>(exp);
        }

        if (begin++->type != LuaToken::Type::THEN) {
            return "stat (if): 'then' expected";
        }

        if (auto then = parse_block(begin, end); holds_alternative<string>(then)) {
            return "stat (if) -> " + get<string>(then);
        } else {
            if_stmt->branches.back().second = get<LuaChunk>(then);
        }

        while (begin->type == LuaToken::Type::ELSEIF) {
            begin++; // elseif

            if_stmt->branches.emplace_back();

            if (auto exp = parse_exp(begin, end); holds_alternative<string>(exp)) {
                return "stat (elseif) -> " + get<string>(exp);
            } else {
                if_stmt->branches.back().first = get<LuaExp>(exp);
            }

            if (begin++->type != LuaToken::Type::THEN) {
                return "stat (elseif): 'then' expected";
            }

            if (auto then = parse_block(begin, end); holds_alternative<string>(then)) {
                return "stat (elseif) -> " + get<string>(then);
            } else {
                if_stmt->branches.back().second = get<LuaChunk>(then);
            }
        }

        if (begin->type == LuaToken::Type::ELSE) {
            begin++; // else

            if (auto then = parse_block(begin, end); holds_alternative<string>(then)) {
                return "stat (else) -> " + get<string>(then);
            } else {
                if_stmt->branches.emplace_back(_LuaValue::True(), get<LuaChunk>(then));
            }
        }

        if (begin++->type != LuaToken::Type::END) {
            return "stat (if): 'end' expected";
        }

        copy(stat_begin, begin, back_inserter(if_stmt->tokens));
        return move(if_stmt);
    }
    case LuaToken::Type::FOR:
    {
        begin++;

        if (begin->type == LuaToken::Type::NAME && (begin+1)->type == LuaToken::Type::ASSIGN) {
            LuaForStmt for_stmt = make_shared<_LuaForStmt>();

            for_stmt->var = make_shared<_LuaName>(*begin++); // name
            begin++; // =

            auto ast = parse_exp(begin, end);
            if (holds_alternative<string>(ast)) {
                return "stat (for) -> " + get<string>(ast);
            } else {
                for_stmt->start = get<LuaExp>(ast);
            }

            if (begin++->type != LuaToken::Type::COMMA) {
                return "stat (for): ',' expected";
            }

            ast = parse_exp(begin, end);
            if (holds_alternative<string>(ast)) {
                return "stat (for) -> " + get<string>(ast);
            } else {
                for_stmt->end = get<LuaExp>(ast);
            }

            if (begin->type == LuaToken::Type::COMMA) {
                begin++; // ,

                ast = parse_exp(begin, end);
                if (holds_alternative<string>(ast)) {
                    return "stat (for) -> " + get<string>(ast);
                } else {
                    for_stmt->step = get<LuaExp>(ast);
                }
            } else {
                for_stmt->step = _LuaValue::Int(1);
            }

            if (begin++->type != LuaToken::Type::DO) {
                return "stat (for): 'do' expected";
            }

            if (auto ast = parse_block(begin, end); holds_alternative<string>(ast)) {
                return "stat (for) -> " + get<string>(ast);
            } else {
                for_stmt->body = get<LuaChunk>(ast);
            }

            if (begin++->type != LuaToken::Type::END) {
                return "stat (for): 'end' expected";
            }

            copy(stat_begin, begin, back_inserter(for_stmt->tokens));
            return move(for_stmt);
        } else {
            return "unimplemented";
        }
    }
    case LuaToken::Type::FUNCTION:
    {
        begin++;

        LuaAssignment assign = make_shared<_LuaAssignment>();

        if (auto name = parse_funcname(begin, end); holds_alternative<string>(name)) {
            return "stat (function): -> " + get<string>(name);
        } else {
            assign->varlist = make_shared<_LuaExplist>();
            assign->varlist->exps.push_back(get<LuaVar>(name));
        }

        if (auto body = parse_funcbody(begin, end); holds_alternative<string>(body)) {
            return "stat (function): -> " + get<string>(body);
        } else {
            assign->explist = make_shared<_LuaExplist>();
            assign->explist->exps.push_back(get<LuaFunction>(body));
        }

        copy(stat_begin, begin, back_inserter(assign->tokens));
        return move(assign);
    }
    case LuaToken::Type::LOCAL:
    {
        begin++;

        LuaAssignment assign = make_shared<_LuaAssignment>();
        assign->local = true;

        if (begin->type == LuaToken::Type::FUNCTION) {
            begin++;

            if (begin->type == LuaToken::Type::NAME) {
                auto name = make_shared<_LuaName>(*begin++);
                assign->varlist = make_shared<_LuaExplist>();
                assign->varlist->exps.push_back(name);
            } else {
                return "stat (local function): name expected.";
            }

            if (auto body = parse_funcbody(begin, end); holds_alternative<string>(body)) {
                return "stat (local function): -> " + get<string>(body);
            } else {
                assign->explist = make_shared<_LuaExplist>();
                assign->explist->exps.push_back(get<LuaFunction>(body));
            }

        } else {
            auto namelist = parse_namelist(begin, end);
            if (holds_alternative<string>(namelist)) {
                return "stat (local assignment) -> " + get<string>(namelist);
            }
            assign->varlist = get<LuaExplist>(namelist);

            if (begin->type == LuaToken::Type::ASSIGN) {
                begin++;
                auto explist = parse_explist(begin, end);
                if (holds_alternative<string>(explist)) {
                    return "stat (local assignment) -> " + get<string>(explist);
                }
                assign->explist = get<LuaExplist>(explist);
            } else {
                assign->explist = make_shared<_LuaExplist>();
            }
        }
        copy(stat_begin, begin, back_inserter(assign->tokens));
        return move(assign);
    }
    default:
        return "stat: wrong alternative " + begin->match;
    }
}

auto LuaParser::parse_laststat(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaStmt> {
    //cout << "laststat" << endl;
    // laststat ::= return [explist] | break

    if (begin == end)
        return "laststat: unexpected end";

    switch (begin->type) {
    case LuaToken::Type::RETURN:
    {
        begin++;

        token_it_t old_begin = begin;
        if (auto ast = parse_explist(begin, end); holds_alternative<LuaExplist>(ast)) {
            return make_shared<_LuaReturnStmt>(get<LuaExplist>(ast));
        } else {
            begin = old_begin;
        }

        return make_shared<_LuaReturnStmt>();
    }
    case LuaToken::Type::BREAK:
        begin++;
        return make_shared<_LuaBreakStmt>();
    default:
        return "laststat: wrong alternative " + begin->match;
    }
}

auto LuaParser::parse_varlist(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist> {
    //cout << "varlist" << endl;
    // varlist ::= var {`,´ var}

    LuaExplist varlist = make_shared<_LuaExplist>();

    do {
        auto ast = parse_prefixexp(begin, end);
        if (holds_alternative<string>(ast)) {
            return "varlist -> " + get<string>(ast);
        } else {
            if (dynamic_pointer_cast<_LuaVar>(get<LuaExp>(ast))) {
                varlist->exps.push_back(get<LuaExp>(ast));
            } else {
                return "varlist: var expected, got functioncall";
            }
        }
    } while (begin++->type == LuaToken::Type::COMMA);

    begin--;

    return move(varlist);
}

auto LuaParser::parse_var(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaVar> {
    //cout << "var" << endl;
    // var ::=  Name | prefixexp `[´ exp `]´ | prefixexp `.´ Name

    if (begin->type == LuaToken::Type::NAME) {
        return make_shared<_LuaNameVar>(make_shared<_LuaName>(*begin++));
    }

    LuaExp prefixexp;
    if (auto ast = parse_prefixexp(begin, end); holds_alternative<string>(ast)) {
        return "var -> " + get<string>(ast);
    } else {
        prefixexp = get<LuaExp>(ast);
    }

//    if (begin->type == LuaToken::Type::LSB) {
//        LuaIndexVar var = make_shared<_LuaIndexVar>();
//        var->table = prefixexp;

//        begin++; // [

//        if (auto ast = parse_exp(begin, end); holds_alternative<string>(ast)) {
//            return "var [] -> " + get<string>(ast);
//        } else {
//            var->index = get<LuaExp>(ast);
//        }

//        if (begin++->type != LuaToken::Type::RSB) {
//            return "var: ']' expected";
//        }

//        return var;
//    } else if (begin->type == LuaToken::Type::DOT) {
//        LuaMemberVar var = make_shared<_LuaMemberVar>();
//        var->table = prefixexp;
//        begin++; // .

//        if (begin++->type != LuaToken::Type::NAME) {
//            return "var: Name expected";
//        } else {
//            var->member = make_shared<_LuaName>(*(begin-1));
//        }

//        return var;
//    }

    return "var: wrong alternative " + begin->match;
}

auto LuaParser::parse_namelist(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist> {
    //cout << "namelist" << endl;
    // namelist ::= Name {`,´ Name}

    LuaExplist namelist = make_shared<_LuaExplist>();

    do {
        if (begin->type == LuaToken::Type::NAME) {
            namelist->exps.push_back(make_shared<_LuaNameVar>(make_shared<_LuaName>(*begin++)));
        } else {
            return "namelist: name expected";
        }
    } while (begin++->type == LuaToken::Type::COMMA && begin->type == LuaToken::Type::NAME);

    begin--;

    return namelist;
}

auto LuaParser::parse_explist(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist> {
    //cout << "explist" << endl;
    // explist ::= {exp `,´} exp

    LuaExplist explist = make_shared<_LuaExplist>();

    do {
        auto ast = parse_exp(begin, end);
        if (holds_alternative<string>(ast)) {
            return "explist -> " + get<string>(ast);
        } else {
            explist->exps.push_back(get<LuaExp>(ast));
        }
    } while (begin++->type == LuaToken::Type::COMMA);

    begin--;

    return explist;
}

LuaExp resolve_precedence(vector<LuaExp>& exps, vector<LuaToken>& ops) {
    //                         Operator     precedence   left associative
    const static unordered_map<LuaToken::Type, pair<int, bool>> precedences = {
        {LuaToken::Type::ADD, {5, true}},
        {LuaToken::Type::SUB, {5, true}},
        {LuaToken::Type::MUL, {6, true}},
        {LuaToken::Type::DIV, {6, true}},
        {LuaToken::Type::POW, {8, false}},
        {LuaToken::Type::MOD, {6, true}},
        {LuaToken::Type::CONCAT, {4, false}},
        {LuaToken::Type::LT, {3, true}},
        {LuaToken::Type::LEQ, {3, true}},
        {LuaToken::Type::GT, {3, true}},
        {LuaToken::Type::GEQ, {3, true}},
        {LuaToken::Type::EQ, {3, true}},
        {LuaToken::Type::NEQ, {3, true}},
        {LuaToken::Type::AND, {2, true}},
        {LuaToken::Type::OR, {1, true}},
        {LuaToken::Type::EVAL, {9, true}}
    };

    for (int i = 0; i < static_cast<int>(ops.size())-1; ++i) {
        while (i < static_cast<int>(ops.size())-1 &&
               precedences.at(ops[i].type).first - precedences.at(ops[i+1].type).first >= (precedences.at(ops[i].type).second ? 0 : 1)) {
            LuaOp op = make_shared<_LuaOp>();
            op->lhs = exps[i];
            op->rhs = exps[i+1];
            op->op = ops[i];
            exps[i+1] = op;

            ops.erase(ops.begin() + i);
            exps.erase(exps.begin() + i);
            i = max(i-1, 0);
        }
    }

    for (int i = ops.size()-1; i >= 0; --i) {
        LuaOp op = make_shared<_LuaOp>();
        op->lhs = exps[i];
        op->rhs = exps[i+1];
        op->op = ops[i];
        exps[i+1] = op;

        ops.erase(ops.begin() + i);
        exps.erase(exps.begin() + i);
    }

    return exps[0];
}

auto LuaParser::parse_exp(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExp> {
    //cout << "exp" << endl;
    // exp ::=  nil | false | true | Number | String | `...´ | function |
    //          prefixexp | tableconstructor | exp binop exp | unop exp

    if (begin == end)
        return "exp: unexpected end";

    vector<LuaExp> exps;
    vector<LuaToken> ops;

    LuaToken unop_token;
    bool is_unop = false;
    for(;;) {
        switch (begin->type) {
        case LuaToken::Type::NIL:
        case LuaToken::Type::FALSE:
        case LuaToken::Type::TRUE:
        case LuaToken::Type::NUMLIT:
        case LuaToken::Type::STRINGLIT:
            exps.push_back(_LuaValue::Value(*begin++));
            break;
        case LuaToken::Type::ELLIPSE:
            begin++;
            return "unimplemented";
        case LuaToken::Type::FUNCTION:
            if (auto func = parse_function(begin, end); holds_alternative<LuaFunction>(func)) {
                exps.push_back(get<LuaFunction>(func));
                break;
            } else {
                return "exp -> " + get<string>(func);
            }
        case LuaToken::Type::LCB: // tableconstructor
            if (auto table = parse_tableconstructor(begin, end); holds_alternative<LuaTableconstructor>(table)) {
                exps.push_back(get<LuaTableconstructor>(table));
                break;
            } else {
                return "exp -> " + get<string>(table);
            }
        case LuaToken::Type::LRB:
        case LuaToken::Type::NAME:
            if (auto ast = parse_prefixexp(begin, end); holds_alternative<LuaExp>(ast)) {
                exps.push_back(get<LuaExp>(ast));
                break;
            } else {
                return "exp -> " + get<string>(ast);
            }
        case LuaToken::Type::SUB:
        case LuaToken::Type::NOT:
        case LuaToken::Type::LEN:
        case LuaToken::Type::STRIP: // unop
            is_unop = true;
            unop_token = *begin++;
            continue;
        default:
            // if the last operator was \, then we can parse it as postfix operator
            // otherwise, the right operand is missing

            if (!ops.empty() && ops.back().type == LuaToken::Type::EVAL) {
                is_unop = true;
                unop_token = ops.back();
                ops.pop_back();
            } else {
                return "wrong alternative " + begin->match;
            }
        }

        if (is_unop) {
            is_unop = false;
            LuaUnop unop = make_shared<_LuaUnop>();
            unop->exp = exps.back();
            unop->op = unop_token;
            exps.back() = unop;
        }

        if (begin->type == LuaToken::Type::ADD ||
            begin->type == LuaToken::Type::SUB ||
            begin->type == LuaToken::Type::MUL ||
            begin->type == LuaToken::Type::DIV ||
            begin->type == LuaToken::Type::POW ||
            begin->type == LuaToken::Type::MOD ||
            begin->type == LuaToken::Type::CONCAT ||
            begin->type == LuaToken::Type::EVAL ||
            begin->type == LuaToken::Type::BANG ||
            begin->type == LuaToken::Type::LT ||
            begin->type == LuaToken::Type::LEQ ||
            begin->type == LuaToken::Type::GT ||
            begin->type == LuaToken::Type::GEQ ||
            begin->type == LuaToken::Type::EQ ||
            begin->type == LuaToken::Type::NEQ ||
            begin->type == LuaToken::Type::AND ||
            begin->type == LuaToken::Type::OR) {

            ops.push_back(*begin++);
        } else {
            break;
        }
    }

    return resolve_precedence(exps, ops);
}

auto LuaParser::parse_prefixexp(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExp> {
    //cout << "prefixexp " << *begin << endl;
    // prefixexp ::= var | functioncall | `(´ exp `)´

    LuaExp result;

    if (begin == end)
        return "prefixexp: unexpected end";

    if (begin->type != LuaToken::Type::LRB) {
        if (auto ast = parse_var(begin, end); holds_alternative<LuaVar>(ast)) {
            result = get<LuaVar>(ast);
        } else {
            return "prefixexp -> " + get<string>(ast);
        }
    } else {
        begin++;

        auto ast = parse_exp(begin, end);
        if (holds_alternative<string>(ast)) {
            return "prefixexp -> " + get<string>(ast);
        }

        if (begin++->type != LuaToken::Type::RRB) {
            return "prefixexp: ')' expected";
        }

        result = get<LuaExp>(ast);
    }

    while (begin != end && (
           begin->type == LuaToken::Type::LRB || begin->type == LuaToken::Type::COLON ||
           begin->type == LuaToken::Type::LCB || begin->type == LuaToken::Type::STRINGLIT ||
           begin->type == LuaToken::Type::LSB || begin->type == LuaToken::Type::DOT)) {

        if (begin->type == LuaToken::Type::LSB) {
            //cout << "idx" << endl;
            LuaIndexVar var = make_shared<_LuaIndexVar>();
            var->table = result;

            begin++; // [

            if (auto ast = parse_exp(begin, end); holds_alternative<string>(ast)) {
                return "var [] -> " + get<string>(ast);
            } else {
                var->index = get<LuaExp>(ast);
            }

            if (begin++->type != LuaToken::Type::RSB) {
                return "var: ']' expected";
            }

            result = var;
        } else if (begin->type == LuaToken::Type::DOT) {
            LuaMemberVar var = make_shared<_LuaMemberVar>();
            var->table = result;
            begin++; // .

            if (begin++->type != LuaToken::Type::NAME) {
                return "var: Name expected";
            } else {
                var->member = make_shared<_LuaName>(*(begin-1));
            }

            result = var;
        } else {
            if (auto call = parse_functioncall(begin, end, result); holds_alternative<string>(call)) {
                return "prefixexp -> " + get<string>(call);
            } else {
                result = get<LuaFunctioncall>(call);
            }
        }
    }

    return move(result);
}

auto LuaParser::parse_functioncall(token_it_t& begin, token_it_t& end, const LuaExp& prefixexp) const -> parse_result_t<LuaFunctioncall> {
    //cout << "functioncall" << endl;
    // functioncall ::=  prefixexp args | prefixexp `:´ Name args
    LuaFunctioncall call = make_shared<_LuaFunctioncall>();

    call->function = prefixexp;

    LuaName name;
    if (begin->type == LuaToken::Type::COLON) {
        begin++; // :

        if (begin++->type == LuaToken::Type::NAME) {
            name = make_shared<_LuaName>(*(begin-1));
        } else {
            return "functioncall: Name expected";
        }
    }

    if (auto ast = parse_args(begin, end); holds_alternative<string>(ast)) {
        return "functioncall -> " + get<string>(ast);
    } else {
        call->args = get<LuaExplist>(ast);
    }

    if (name) {
        call->args->exps.insert(call->args->exps.begin(), make_shared<_LuaNameVar>(name));
    }

    return call;
}

auto LuaParser::parse_args(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaExplist> {
    //cout << "args" << endl;
    // args ::=  `(´ [explist] `)´ | tableconstructor | String

    if (begin == end)
        return "args: unexpected end";

    LuaExplist args = make_shared<_LuaExplist>();

    switch (begin->type) {
    case LuaToken::Type::LRB:
        begin++; // (

        if (begin->type != LuaToken::Type::RRB) {
            auto ast = parse_explist(begin, end);
            if (holds_alternative<string>(ast)) {
                return "args -> " + get<string>(ast);
            } else {
                args = get<LuaExplist>(ast);
            }
        }

        if (begin++->type != LuaToken::Type::RRB) {
            return "args: ) expected";
        }

        return args;
    case LuaToken::Type::LCB:
    {
        auto ast = parse_tableconstructor(begin, end);
        if (holds_alternative<string>(ast)) {
            return "args -> " + get<string>(ast);
        } else {
            args->exps.push_back(get<LuaTableconstructor>(ast));
        }
        return args;
    }
    case LuaToken::Type::STRINGLIT:
        args->exps.push_back(make_shared<_LuaValue>(*begin++));

        return args;
    default:
        return "args: wrong alternative " + begin->match;
    }
}

auto LuaParser::parse_function(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaFunction> {
    //cout << "function" << endl;
    // function ::= function funcbody

    if (begin == end)
        return "function: unexpected end";

    if (begin++->type != LuaToken::Type::FUNCTION) {
        return "function: 'function' expected";
    }

    return parse_funcbody(begin, end);
}

auto LuaParser::parse_funcbody(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaFunction> {
    //cout << "funcbody" << endl;
    // funcbody ::= `(´ [parlist] `)´ block end

    if (begin == end)
        return "funcbody: unexpected end";

    LuaFunction func = make_shared<_LuaFunction>();

    if (begin++->type != LuaToken::Type::LRB) {
        return "funcbody: ( expected";
    }

    if (begin->type != LuaToken::Type::RRB) {
        if (auto parlist = parse_parlist(begin, end); holds_alternative<string>(parlist)) {
            return "funcbody -> " + get<string>(parlist);
        } else {
            func->params = get<LuaExplist>(parlist);
        }
    } else {
        func->params = make_shared<_LuaExplist>();
    }

    if (begin++->type != LuaToken::Type::RRB) {
        return "funcbody: ) expected";
    }

    if (auto block = parse_block(begin, end); holds_alternative<string>(block)) {
        return "funcbody -> " + get<string>(block);
    } else {
        func->body = get<LuaChunk>(block);
    }

    if (begin++->type != LuaToken::Type::END) {
        return "funcbody: 'end' expected";
    }

    return func;
}

auto LuaParser::parse_parlist(token_it_t &begin, token_it_t &end) const -> parse_result_t<LuaExplist> {
    //cout << "parlist" << endl;
    // parlist ::= namelist [`,´ `...´] | `...´

    if (begin == end)
        return "parlist: unexpected end";

    if (begin->type == LuaToken::Type::ELLIPSE) {
        LuaExplist result = make_shared<_LuaExplist>();
        result->exps.push_back(make_shared<_LuaValue>(*begin++));
        return result;
    }

    LuaExplist parlist;
    if (auto namelist = parse_namelist(begin, end); holds_alternative<LuaExplist>(namelist)) {
        parlist = get<LuaExplist>(namelist);
    } else {
        return "parlist -> " + get<string>(namelist);
    }

    if (begin->type == LuaToken::Type::COMMA) {
        begin++; // comma
        if (begin++->type == LuaToken::Type::ELLIPSE) {
            parlist->exps.push_back(make_shared<_LuaValue>(*begin++));
        } else {
            return "parlist: ... expected";
        }
    }

    return parlist;
}

auto LuaParser::parse_tableconstructor(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaTableconstructor> {
    //cout << "tableconstructor" << endl;
    // tableconstructor ::= `{´ [fieldlist] `}´
    // fieldlist ::= field {fieldsep field} [fieldsep]

    if (begin == end)
        return "tableconstructor: unexpected end";

    if (begin++->type != LuaToken::Type::LCB) {
        return "tableconstructor: '{' expected";
    }

    LuaTableconstructor result = make_shared<_LuaTableconstructor>();
    auto tableconst_begin = begin-1;

    if (begin->type != LuaToken::Type::RCB) {
        if (auto field = parse_field(begin, end); holds_alternative<LuaField>(field)) {
            result->fields.push_back(get<LuaField>(field));
        } else {
            return "tableconstructor -> " + get<string>(field);
        }
    }

    while (begin->type == LuaToken::Type::SEM || begin->type == LuaToken::Type::COMMA) {
        begin++; // ; bzw ,

        if (begin->type == LuaToken::Type::RCB) {
            break;
        }

        if (auto field = parse_field(begin, end); holds_alternative<LuaField>(field)) {
            result->fields.push_back(get<LuaField>(field));
        } else {
            return "tableconstructor -> " + get<string>(field);
        }
    }

    if (begin++->type != LuaToken::Type::RCB) {
        return "tableconstructor: '}' expected";
    }

    // add all tokens
    for (auto it = tableconst_begin; it != begin; ++it)
        result->tokens.push_back(*it);

    return move(result);
}

auto LuaParser::parse_field(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaField> {
    //cout << "field" << endl;
    // field ::= `[´ exp `]´ `=´ exp | Name `=´ exp | exp

    if (begin == end)
        return "field: unexpected end";

    LuaField field = make_shared<_LuaField>();

    if (begin->type == LuaToken::Type::LSB) {
        begin++; // [

        if (auto exp = parse_exp(begin, end); holds_alternative<LuaExp>(exp)) {
            field->lhs = get<LuaExp>(exp);
        } else {
            return "field -> " + get<string>(exp);
        }

        if (begin++->type != LuaToken::Type::RSB) {
            return "field: ']' expected";
        }

        if (begin++->type != LuaToken::Type::ASSIGN) {
            return "field: '=' expected";
        }

    } else if (begin->type == LuaToken::Type::NAME) {
        if (begin+1 != end && (begin+1)->type == LuaToken::Type::ASSIGN) {
            field->lhs = make_shared<_LuaName>(*begin);
            begin++; // name
            begin++; // =
        }
    }

    if (auto exp = parse_exp(begin, end); holds_alternative<LuaExp>(exp)) {
        field->rhs = get<LuaExp>(exp);
    } else {
        return "field -> " + get<string>(exp);
    }

    return field;
}

auto LuaParser::parse_funcname(token_it_t& begin, token_it_t& end) const -> parse_result_t<LuaVar> {
    //cout << "funcname" << endl;
    // var ::=  funcname ::= Name {`.´ Name} [`:´ Name]

    if (begin->type == LuaToken::Type::NAME) {
        return make_shared<_LuaNameVar>(make_shared<_LuaName>(*begin++));
    }

    return "funcname: name expected";
}

string get_string(const LuaParser::token_list_t& tokens) {
    stringstream ss;

    for (const auto& t : tokens) {
        ss << t.ws << t.match;
    }

    return ss.str();
}
