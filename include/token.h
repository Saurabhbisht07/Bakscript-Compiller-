#ifndef TOKEN_H
#define TOKEN_H
typedef enum
{
    // Data types / type keywords
    TOKEN_NUM,
    TOKEN_STR,
    TOKEN_BOOL_TYPE,
    TOKEN_ARRAY_TYPE,
    TOKEN_OBJECT_TYPE,
    TOKEN_AUTO,
    // Literals
    TOKEN_TRUE,
    TOKEN_FALSE,
    // Operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_NOT,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_LESS_EQ,
    TOKEN_GREATER_EQ,
    TOKEN_EQ_EQ,
    TOKEN_NOT_EQ,
    TOKEN_AND_AND,
    TOKEN_OR_OR,
    // Keywords
    TOKEN_SHOW,      // print
    TOKEN_WHEN,      // if
    TOKEN_OTHERWISE, // else
    TOKEN_REPEAT,    // for
    TOKEN_WHILE,     // while
    TOKEN_FUNCTION, // function declaration
    TOKEN_RETURN,   // return from function
    TOKEN_ASK,       // input
    // Others
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_COMMA,
    TOKEN_EQUALS,
    TOKEN_SEMICOLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_EOF
} TokenType;

typedef struct
{
    TokenType type;
    char *value;
    int line;
    int column;
} Token;

#endif