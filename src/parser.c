#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/parser.h"

const char *token_type_to_string(TokenType type)
{
    switch (type)
    {
    case TOKEN_NUM:
        return "NUM";
    case TOKEN_STR:
        return "STR";
    case TOKEN_BOOL_TYPE:
        return "BOOL";
    case TOKEN_ARRAY_TYPE:
        return "ARRAY";
    case TOKEN_OBJECT_TYPE:
        return "OBJECT";
    case TOKEN_AUTO:
        return "AUTO";
    case TOKEN_TRUE:
        return "TRUE";
    case TOKEN_FALSE:
        return "FALSE";
    case TOKEN_PLUS:
        return "PLUS";
    case TOKEN_MINUS:
        return "MINUS";
    case TOKEN_MULTIPLY:
        return "MULTIPLY";
    case TOKEN_DIVIDE:
        return "DIVIDE";
    case TOKEN_NOT:
        return "NOT";
    case TOKEN_LESS:
        return "LESS";
    case TOKEN_GREATER:
        return "GREATER";
    case TOKEN_LESS_EQ:
        return "LESS_EQ";
    case TOKEN_GREATER_EQ:
        return "GREATER_EQ";
    case TOKEN_EQ_EQ:
        return "EQ_EQ";
    case TOKEN_NOT_EQ:
        return "NOT_EQ";
    case TOKEN_AND_AND:
        return "AND_AND";
    case TOKEN_OR_OR:
        return "OR_OR";
    case TOKEN_SHOW:
        return "SHOW";
    case TOKEN_WHEN:
        return "WHEN";
    case TOKEN_OTHERWISE:
        return "OTHERWISE";
    case TOKEN_REPEAT:
        return "REPEAT";
    case TOKEN_WHILE:
        return "WHILE";
    case TOKEN_FUNCTION:
        return "FUNCTION";
    case TOKEN_RETURN:
        return "RETURN";
    case TOKEN_ASK:
        return "ASK";
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_NUMBER_LITERAL:
        return "NUMBER_LITERAL";
    case TOKEN_STRING_LITERAL:
        return "STRING_LITERAL";
    case TOKEN_EQUALS:
        return "EQUALS";
    case TOKEN_SEMICOLON:
        return "SEMICOLON";
    case TOKEN_COMMA:
        return "COMMA";
    case TOKEN_LPAREN:
        return "LPAREN";
    case TOKEN_RPAREN:
        return "RPAREN";
    case TOKEN_LBRACKET:
        return "LBRACKET";
    case TOKEN_RBRACKET:
        return "RBRACKET";
    case TOKEN_LBRACE:
        return "LBRACE";
    case TOKEN_RBRACE:
        return "RBRACE";
    case TOKEN_EOF:
        return "EOF";
    default:
        return "UNKNOWN";
    }
}

Parser *create_parser(Lexer *lexer)
{
    Parser *parser = (Parser *)malloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->current_token = lexer_get_next_token(lexer);
    parser->allocated_tokens = NULL;
    parser->token_count = 0;
    parser->token_capacity = 0;

    // Track first token so free_parser can release token->value too.
    // (lexer_get_next_token allocates Token and Token->value via strdup)
    if (parser->current_token)
    {
        parser->token_capacity = 64;
        parser->allocated_tokens = (Token **)malloc(sizeof(Token *) * (size_t)parser->token_capacity);
        if (!parser->allocated_tokens)
        {
            fprintf(stderr, "Error: Memory allocation failed for parser token tracking\n");
            exit(1);
        }
        parser->allocated_tokens[parser->token_count++] = parser->current_token;
    }
    return arser;
}

void parser_advance(Parser *parser)
{
    Token *next = lexer_get_next_token(parser->lexer);
    if (parser->token_count >= parser->token_capacity)
    {
        int new_cap = parser->token_capacity == 0 ? 64 : parser->token_capacity * 2;
        Token **new_arr = (Token **)realloc(parser->allocated_tokens, sizeof(Token *) * (size_t)new_cap);
        if (!new_arr)
        {
            fprintf(stderr, "Error: Memory reallocation failed in parser token tracking\n");
            exit(1);
        }
        parser->allocated_tokens = new_arr;
        parser->token_capacity = new_cap;
    }
    if (next)
        parser->allocated_tokens[parser->token_count++] = next;
    parser->current_token = next;
}
p
bool parser_expect(Parser *parser, TokenType type)
{
    if (parser->current_token->type == type)
    {
        parser_advance(parser);
        return true;
    }
    else
    {
        fprintf(stderr, "Error: Expected %s but got '%s' (%s) at line %d, column %d\n",
                token_type_to_string(type),
                parser->current_token->value ? parser->current_token->value : "EOF",
                token_type_to_string(parser->current_token->type),
                parser->current_token->line,
                parser->current_token->column);
        return false;
    }
}

void free_parser(Parser *parser)
{
    if (!parser)
        return;

    if (parser->allocated_tokens)
    {
        for (int i = 0; i < parser->token_count; i++)
        {
            Token *t = parser->allocated_tokens[i];
            if (!t)
                continue;
            free(t->value);
            free(t);
        }
        free(parser->allocated_tokens);
    }
    free(parser);
}

// Forward declarations for recursive descent
static Node *parse_block(Parser *parser);
static Node *parse_postfix(Parser *parser, Node *base);

static Node *parse_array_literal(Parser *parser)
{
    Token *start = parser->current_token;
    if (!parser_expect(parser, TOKEN_LBRACKET))
        return NULL;

    Node **elements = NULL;
    int count = 0;
    int capacity = 0;

    if (parser->current_token->type != TOKEN_RBRACKET)
    {
        while (1)
        {
            if (count >= capacity)
            {
                capacity = capacity == 0 ? 4 : capacity * 2;
                Node **new_arr = (Node **)realloc(elements, sizeof(Node *) * (size_t)capacity);
                if (!new_arr)
                {
                    for (int i = 0; i < count; i++)
                        free_node(elements[i]);
                    free(elements);
                    return NULL;
                }
                elements = new_arr;
            }

            elements[count] = parse_expression(parser);
            if (!elements[count])
            {
                for (int i = 0; i < count; i++)
                    free_node(elements[i]);
                free(elements);
                return NULL;
            }
            count++;

            if (parser->current_token->type == TOKEN_COMMA)
            {
                parser_advance(parser);
                continue;
            }
            break;
        }
    }

    if (!parser_expect(parser, TOKEN_RBRACKET))
    {
        for (int i = 0; i < count; i++)
            free_node(elements[i]);
        free(elements);
        return NULL;
    }

    Node *arr = create_array_literal_node(elements, count, start->line, start->column);
    free(elements);
    return arr;
}

static Node *parse_postfix(Parser *parser, Node *base)
{
    Node *result = base;
    while (parser->current_token->type == TOKEN_LBRACKET)
    {
        Token *tok = parser->current_token;
        parser_advance(parser);
        Node *index = parse_expression(parser);
        if (!index)
        {
            free_node(result);
            return NULL;
        }
        if (!parser_expect(parser, TOKEN_RBRACKET))
        {
            free_node(result);
            free_node(index);
            return NULL;
        }
        result = create_index_access_node(result, index, tok->line, tok->column);
    }
    return result;
}

Node *parse_primary(Parser *parser)
{
    Token *token = parser->current_token;

    switch (token->type)
    {
    case TOKEN_NUMBER_LITERAL:
        parser_advance(parser);
        return create_number_node(atoi(token->value), token->line, token->column);

    case TOKEN_TRUE:
        parser_advance(parser);
        return create_bool_node(1, token->line, token->column);

    case TOKEN_FALSE:
        parser_advance(parser);
        return create_bool_node(0, token->line, token->column);

    case TOKEN_STRING_LITERAL:
        parser_advance(parser);
        return create_string_node(token->value, token->line, token->column);

    case TOKEN_IDENTIFIER:
        parser_advance(parser);
        if (parser->current_token->type == TOKEN_LPAREN)
        {
            return parse_function_call(parser, token);
        }
    {
        Node *id = create_identifier_node(token->value, token->line, token->column);
        return parse_postfix(parser, id);
    }

    case TOKEN_SHOW:
    case TOKEN_ASK:
        return parse_function_call(parser, token);

    case TOKEN_LPAREN:
        parser_advance(parser);
        Node *expr = parse_expression(parser);
        if (!expr)
            return NULL;
        if (!parser_expect(parser, TOKEN_RPAREN))
        {
            free_node(expr);
            return NULL;
        }
        return parse_postfix(parser, expr);

    case TOKEN_LBRACKET:
    {
        Node *arr = parse_array_literal(parser);
        if (!arr)
            return NULL;
        return parse_postfix(parser, arr);
    }

    default:
        return NULL;
    }
}

// Parse a factor (unary operations and primary expressions)
Node *parse_factor(Parser *parser)
{
    // Unary operators have higher precedence than multiplication.
    if (parser->current_token->type == TOKEN_NOT)
    {
        Token *op_token = parser->current_token;
        parser_advance(parser);
        Node *operand = parse_factor(parser);
        if (!operand)
            return NULL;
        return create_unary_op_node(UOP_NOT, operand, op_token->line, op_token->column);
    }
    return parse_primary(parser);
}

// Parse a term (multiplication and division)
Node *parse_term(Parser *parser)
{
    Node *left = parse_factor(parser);
    if (!left)
        return NULL;

    while (parser->current_token->type == TOKEN_MULTIPLY ||
           parser->current_token->type == TOKEN_DIVIDE)
    {
        Token *op_token = parser->current_token;
        BinaryOpType op = (op_token->type == TOKEN_MULTIPLY) ? OP_MULTIPLY : OP_DIVIDE;
        parser_advance(parser);

        Node *right = parse_factor(parser);
        if (!right)
            return NULL;

        left = create_binary_op_node(op, left, right, op_token->line, op_token->column);
    }

    return left;
}

// Parse a binary operator chain with left associativity.
static Node *parse_addition(Parser *parser)
{
    Node *left = parse_term(parser);
    if (!left)
        return NULL;

    while (parser->current_token->type == TOKEN_PLUS ||
           parser->current_token->type == TOKEN_MINUS)
    {
        Token *op_token = parser->current_token;
        BinaryOpType op = (op_token->type == TOKEN_PLUS) ? OP_ADD : OP_SUBTRACT;
        parser_advance(parser);

        Node *right = parse_term(parser);
        if (!right)
            return NULL;

        left = create_binary_op_node(op, left, right, op_token->line, op_token->column);
    }
    return left;
}

static Node *parse_comparison(Parser *parser)
{
    Node *left = parse_addition(parser);
    if (!left)
        return NULL;

    while (parser->current_token->type == TOKEN_LESS ||
           parser->current_token->type == TOKEN_GREATER ||
           parser->current_token->type == TOKEN_LESS_EQ ||
           parser->current_token->type == TOKEN_GREATER_EQ)
    {
        Token *op_token = parser->current_token;
        BinaryOpType op;
        switch (op_token->type)
        {
        case TOKEN_LESS:
            op = OP_LESS;
            break;
        case TOKEN_GREATER:
            op = OP_GREATER;
            break;
        case TOKEN_LESS_EQ:
            op = OP_LESS_EQ;
            break;
        case TOKEN_GREATER_EQ:
            op = OP_GREATER_EQ;
            break;
        default:
            return NULL;
        }

        parser_advance(parser);
        Node *right = parse_addition(parser);
        if (!right)
            return NULL;

        left = create_binary_op_node(op, left, right, op_token->line, op_token->column);
    }
    return left;
}

static Node *parse_equality(Parser *parser)
{
    Node *left = parse_comparison(parser);
    if (!left)
        return NULL;

    while (parser->current_token->type == TOKEN_EQ_EQ ||
           parser->current_token->type == TOKEN_NOT_EQ)
    {
        Token *op_token = parser->current_token;
        BinaryOpType op = (op_token->type == TOKEN_EQ_EQ) ? OP_EQUAL : OP_NOT_EQUAL;
        parser_advance(parser);

        Node *right = parse_comparison(parser);
        if (!right)
            return NULL;

        left = create_binary_op_node(op, left, right, op_token->line, op_token->column);
    }
    return left;
}

static Node *parse_logical_and(Parser *parser)
{
    Node *left = parse_equality(parser);
    if (!left)
        return NULL;

    while (parser->current_token->type == TOKEN_AND_AND)
    {
        Token *op_token = parser->current_token;
        parser_advance(parser);
        Node *right = parse_equality(parser);
        if (!right)
            return NULL;
        left = create_binary_op_node(OP_AND, left, right, op_token->line, op_token->column);
    }
    return left;
}

static Node *parse_logical_or(Parser *parser)
{
    Node *left = parse_logical_and(parser);
    if (!left)
        return NULL;

    while (parser->current_token->type == TOKEN_OR_OR)
    {
        Token *op_token = parser->current_token;
        parser_advance(parser);
        Node *right = parse_logical_and(parser);
        if (!right)
            return NULL;
        left = create_binary_op_node(OP_OR, left, right, op_token->line, op_token->column);
    }
    return left;
}

// Parse an expression with precedence:
//   or -> and -> equality -> comparison -> addition -> term -> unary -> primary
Node *parse_expression(Parser *parser)
{
    return parse_logical_or(parser);
}

// Parse a block of statements
static Node *parse_block(Parser *parser)
{
    Token *start_token = parser->current_token;
    if (!parser_expect(parser, TOKEN_LBRACE))
        return NULL;

    Node **statements = NULL;
    int count = 0;
    int capacity = 0;

    while (parser->current_token->type != TOKEN_RBRACE)
    {
        if (count >= capacity)
        {
            capacity = capacity == 0 ? 4 : capacity * 2;
            statements = realloc(statements, capacity * sizeof(Node *));
        }

        Node *stmt = parse_statement(parser);
        if (!stmt)
        {
            free(statements);
            return NULL;
        }

        statements[count++] = stmt;
    }

    if (!parser_expect(parser, TOKEN_RBRACE))
    {
        free(statements);
        return NULL;
    }

    return create_block_node(statements, count, start_token->line, start_token->column);
}

// Parse a variable declaration
Node *parse_variable_declaration(Parser *parser)
{
    Token *type_token = parser->current_token;
    const char *type = type_token->value;
    parser_advance(parser);

    Token *name_token = parser->current_token;
    if (!parser_expect(parser, TOKEN_IDENTIFIER))
        return NULL;
    const char *name = name_token->value;

    Node *initializer = NULL;
    if (parser->current_token->type == TOKEN_EQUALS)
    {
        parser_advance(parser);
        initializer = parse_expression(parser);
        if (!initializer)
            return NULL;

        if (!parser_expect(parser, TOKEN_SEMICOLON))
            return NULL;

        return create_var_decl_node(type, name, initializer, type_token->line, type_token->column);
    }

    if (!parser_expect(parser, TOKEN_SEMICOLON))
        return NULL;

    return create_var_decl_node(type, name, initializer, type_token->line, type_token->column);
}

// Parse an if statement
Node *parse_if_statement(Parser *parser)
{
    Token *if_token = parser->current_token;
    parser_advance(parser); 

    if (!parser_expect(parser, TOKEN_LPAREN))
        return NULL;

    Node *condition = parse_expression(parser);
    if (!condition)
        return NULL;

    if (!parser_expect(parser, TOKEN_RPAREN))
        return NULL;

    Node *if_body = parse_block(parser);
    if (!if_body)
        return NULL;

    Node *else_body = NULL;
    if (parser->current_token->type == TOKEN_OTHERWISE)
    {
        parser_advance(parser);
        else_body = parse_block(parser);
        if (!else_body)
            return NULL;
    }

    return create_if_node(condition, if_body, else_body, if_token->line, if_token->column);
}

// Parse an assignment expression (for loop increment)
Node *parse_assignment_expression(Parser *parser)
{
    Token *id_token = parser->current_token;
    if (id_token->type != TOKEN_IDENTIFIER)
    {
        return parse_expression(parser);
    }

    Node *left = create_identifier_node(id_token->value, id_token->line, id_token->column);
    parser_advance(parser);

    if (parser->current_token->type == TOKEN_EQUALS)
    {
        Token *op_token = parser->current_token;
        parser_advance(parser);
        Node *right = parse_expression(parser);
        if (!right)
            return NULL;

        return create_binary_op_node(OP_ASSIGN, left, right, op_token->line, op_token->column);
    }

    return left;
}

// Parse a for loop
Node *parse_for_loop(Parser *parser)
{
    Token *for_token = parser->current_token;
    parser_advance(parser); 

    if (!parser_expect(parser, TOKEN_LPAREN))
        return NULL;

    Node *initializer = parse_variable_declaration(parser);
    if (!initializer)
        return NULL;

    Node *condition = parse_expression(parser);
    if (!condition)
        return NULL;

    if (!parser_expect(parser, TOKEN_SEMICOLON))
        return NULL;
    Node *increment = parse_assignment_expression(parser);
    if (!increment)
        return NULL;

    if (!parser_expect(parser, TOKEN_RPAREN))
        return NULL;

    Node *body = parse_block(parser);
    if (!body)
        return NULL;

    return create_for_node(initializer, condition, increment, body, for_token->line, for_token->column);
}

// Parse a while loop: while (condition) { ... }
Node *parse_while_statement(Parser *parser)
{
    Token *while_token = parser->current_token;
    parser_advance(parser);

    if (!parser_expect(parser, TOKEN_LPAREN))
        return NULL;

    Node *condition = parse_expression(parser);
    if (!condition)
        return NULL;

    if (!parser_expect(parser, TOKEN_RPAREN))
        return NULL;

    Node *body = parse_block(parser);
    if (!body)
        return NULL;

    return create_while_node(condition, body, while_token->line, while_token->column);
}

// Parse a function declaration:
// function name(a, b) { return a + b; }
static Node *parse_function_declaration(Parser *parser)
{
    Token *fn_token = parser->current_token;
    parser_advance(parser);

    Token *name_token = parser->current_token;
    if (!parser_expect(parser, TOKEN_IDENTIFIER))
        return NULL;

    const char *fn_name = name_token->value;

    if (!parser_expect(parser, TOKEN_LPAREN))
        return NULL;

    char **params = NULL;
    int param_count = 0;
    int capacity = 0;

    if (parser->current_token->type != TOKEN_RPAREN)
    {
        while (1)
        {
            if (param_count >= capacity)
            {
                capacity = capacity == 0 ? 4 : capacity * 2;
                char **new_params = (char **)realloc(params, (size_t)capacity * sizeof(char *));
                if (!new_params)
                {
                    free(params);
                    return NULL;
                }
                params = new_params;
            }

            Token *param_token = parser->current_token;
            if (!parser_expect(parser, TOKEN_IDENTIFIER))
            {
                free(params);
                return NULL;
            }
            params[param_count++] = param_token->value;

            if (parser->current_token->type == TOKEN_COMMA)
            {
                parser_advance(parser);
                continue;
            }
            break;
        }
    }

    if (!parser_expect(parser, TOKEN_RPAREN))
    {
        if (params)
            free(params);
        return NULL;
    }

    Node *body = parse_block(parser);
    if (!body)
    {
        if (params)
            free(params);
        return NULL;
    }

    Node *fn_decl = create_function_decl_node(fn_name, params, param_count, body, fn_token->line, fn_token->column);
    if (params)
        free(params);
    return fn_decl;
}

// Parse `return expr;`
static Node *parse_return_statement(Parser *parser)
{
    Token *ret_token = parser->current_token;
    parser_advance(parser);

    Node *value = parse_expression(parser);
    if (!value)
        return NULL;

    if (!parser_expect(parser, TOKEN_SEMICOLON))
    {
        free_node(value);
        return NULL;
    }

    return create_return_node(value, ret_token->line, ret_token->column);
}

// Parse a function call (show or ask)
Node *parse_function_call(Parser *parser, Token *func_token)
{
    if (!func_token)
        return NULL;

    // Support both call contexts:
    // 1) parser->current_token is the function keyword/identifier (e.g. SHOW)
    // 2) parser->current_token is already LPAREN (e.g. after parsing identifier in parse_primary)
    if (parser->current_token->type != TOKEN_LPAREN)
    {
        parser_advance(parser);
    }

    if (!parser_expect(parser, TOKEN_LPAREN))
        return NULL;

    Node **args = NULL;
    int arg_count = 0;
    int capacity = 0;

    if (parser->current_token->type != TOKEN_RPAREN)
    {
        while (1)
        {
            if (arg_count >= capacity)
            {
                capacity = capacity == 0 ? 4 : capacity * 2;
                Node **new_args = (Node **)realloc(args, (size_t)capacity * sizeof(Node *));
                if (!new_args)
                {
                    if (args)
                    {
                        for (int i = 0; i < arg_count; i++)
                            free_node(args[i]);
                    }
                    free(args);
                    return NULL;
                }
                args = new_args;
            }

            args[arg_count] = parse_expression(parser);
            if (!args[arg_count])
            {
                for (int i = 0; i < arg_count; i++)
                    free_node(args[i]);
                free(args);
                return NULL;
            }
            arg_count++;

            if (parser->current_token->type == TOKEN_COMMA)
            {
                parser_advance(parser);
                continue;
            }
            break;
        }
    }

    if (!parser_expect(parser, TOKEN_RPAREN))
    {
        if (args)
        {
            for (int i = 0; i < arg_count; i++)
                free_node(args[i]);
            free(args);
        }
        return NULL;
    }

    return create_function_call_node(func_token->value, args, arg_count, func_token->line, func_token->column);
}

// Parse a statement
Node *parse_statement(Parser *parser)
{
    switch (parser->current_token->type)
    {
    case TOKEN_NUM:
    case TOKEN_STR:
    case TOKEN_BOOL_TYPE:
    case TOKEN_ARRAY_TYPE:
    case TOKEN_OBJECT_TYPE:
    case TOKEN_AUTO:
        return parse_variable_declaration(parser);

    case TOKEN_WHEN:
        return parse_if_statement(parser);

    case TOKEN_REPEAT:
        return parse_for_loop(parser);

    case TOKEN_WHILE:
        return parse_while_statement(parser);

    case TOKEN_FUNCTION:
        return parse_function_declaration(parser);

    case TOKEN_RETURN:
        return parse_return_statement(parser);

    case TOKEN_SHOW:
    case TOKEN_ASK:
    {
        Node *func_call = parse_function_call(parser, parser->current_token);
        if (!parser_expect(parser, TOKEN_SEMICOLON))
        {
            free_node(func_call);
            return NULL;
        }
        return func_call;
    }

    case TOKEN_IDENTIFIER:
    {
        Token *token = parser->current_token;
        parser_advance(parser);

        if (parser->current_token->type == TOKEN_LPAREN)
        {
            Node *func_call = parse_function_call(parser, token);
            if (!func_call)
                return NULL;
            if (!parser_expect(parser, TOKEN_SEMICOLON))
            {
                free_node(func_call);
                return NULL;
            }
            return func_call;
        }

        Node *left = create_identifier_node(token->value, token->line, token->column);
        left = parse_postfix(parser, left);
        if (!left)
            return NULL;

        if (parser->current_token->type == TOKEN_EQUALS)
        {
            Token *op_token = parser->current_token;
            parser_advance(parser);
            Node *right = parse_expression(parser);
            if (!right)
            {
                free_node(left);
                return NULL;
            }
            if (!parser_expect(parser, TOKEN_SEMICOLON))
            {
                free_node(left);
                free_node(right);
                return NULL;
            }
            return create_binary_op_node(OP_ASSIGN, left, right, op_token->line, op_token->column);
        }
        else
        {
            // Reject standalone identifiers with a more user-friendly message
            fprintf(stderr, "Error: Invalid statement at line %d, column %d.\n"
                            "The identifier '%s' must be used in a proper statement like:\n"
                            "  - Variable declaration: num %s = value;\n"
                            "  - Assignment: %s = value;\n"
                            "  - Function call: %s(value);\n",
                    token->line, token->column,
                    token->value, token->value, token->value, token->value);
            free_node(left);
            return NULL;
        }
    }

    default:
        fprintf(stderr, "Error: Unexpected token '%s' (%s) in statement at line %d, column %d\n",
                parser->current_token->value ? parser->current_token->value : "EOF",
                token_type_to_string(parser->current_token->type),
                parser->current_token->line,
                parser->current_token->column);
        return NULL;
    }
}

// Parse the entire program
Node *parse_program(Parser *parser)
{
    Node **statements = NULL;
    int count = 0;

    while (parser->current_token->type != TOKEN_EOF)
    {
        statements = (Node **)realloc(statements, (count + 1) * sizeof(Node *));
        Node *stmt = parse_statement(parser);
        if (!stmt)
        {
            for (int i = 0; i < count; i++)
            {
                free_node(statements[i]);
            }
            free(statements);
            return NULL;
        }
        statements[count++] = stmt;
    }

    return create_program_node(statements, count);
}