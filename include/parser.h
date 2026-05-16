#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include <stdbool.h>

typedef struct
{
    Lexer *lexer;
    Token *current_token;
    // Track all tokens allocated during parsing so we can free them safely
    // without requiring parser_advance callers to manage token lifetimes.
    Token **allocated_tokens;
    int token_count;
    int token_capacity;
} Parser;

Parser *create_parser(Lexer *lexer);
void parser_advance(Parser *parser);
bool parser_expect(Parser *parser, TokenType type);
void free_parser(Parser *parser);
Node *parse_program(Parser *parser);
Node *parse_statement(Parser *parser);
Node *parse_variable_declaration(Parser *parser);
Node *parse_if_statement(Parser *parser);
Node *parse_for_loop(Parser *parser);
Node *parse_while_statement(Parser *parser);
Node *parse_function_call(Parser *parser, Token *func_token);
Node *parse_expression(Parser *parser);
Node *parse_term(Parser *parser);
Node *parse_factor(Parser *parser);
Node *parse_primary(Parser *parser);

#endif // PARSER_H