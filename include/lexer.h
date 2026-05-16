#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct
{
    char *source;
    int position;
    int line;
    int column;
    char current_char;
} Lexer;

Lexer *create_lexer(char *source);
void lexer_advance(Lexer *lexer);
void lexer_skip_whitespace(Lexer *lexer);
Token *lexer_get_next_token(Lexer *lexer);
Token *create_token(TokenType type, const char *value, int line, int column);
char *lexer_collect_string(Lexer *lexer);
char *lexer_collect_number(Lexer *lexer);
char *lexer_collect_identifier(Lexer *lexer);
void free_lexer(Lexer *lexer);

#endif 