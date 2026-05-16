#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/semantic.h"
#include "../include/interpreter.h"
#include "../include/tac.h"
#include "../include/gen.h"

void print_token(Token *token)
{
    const char *type_names[] = {
        "TOKEN_NUM", "TOKEN_STR", "TOKEN_TRUE", "TOKEN_FALSE",
        "TOKEN_PLUS", "TOKEN_MINUS", "TOKEN_MULTIPLY", "TOKEN_DIVIDE", "TOKEN_NOT",
        "TOKEN_LESS", "TOKEN_GREATER", "TOKEN_LESS_EQ", "TOKEN_GREATER_EQ",
        "TOKEN_EQ_EQ", "TOKEN_NOT_EQ", "TOKEN_AND_AND", "TOKEN_OR_OR",
        "TOKEN_SHOW", "TOKEN_WHEN", "TOKEN_OTHERWISE", "TOKEN_REPEAT", "TOKEN_WHILE",
        "TOKEN_FUNCTION", "TOKEN_RETURN", "TOKEN_ASK",
        "TOKEN_IDENTIFIER", "TOKEN_NUMBER_LITERAL", "TOKEN_STRING_LITERAL",
        "TOKEN_COMMA", "TOKEN_EQUALS", "TOKEN_SEMICOLON",
        "TOKEN_LPAREN", "TOKEN_RPAREN", "TOKEN_LBRACKET", "TOKEN_RBRACKET",
        "TOKEN_LBRACE", "TOKEN_RBRACE",
        "TOKEN_EOF"};
    printf("Token(type=%s, value='%s', line=%d, column=%d)\n",
           type_names[token->type], token->value ? token->value : "", token->line, token->column);
}

void print_ast(Node *node, int indent)
{
    if (!node)
    {
        for (int i = 0; i < indent; i++)
            printf("  ");
        printf("<null>\n");
        return;
    }

    for (int i = 0; i < indent; i++)
        printf("  ");

    switch (node->type)
    {
    case NODE_NUMBER:
        printf("Number: %d\n", node->number.value);
        break;

    case NODE_STRING:
        printf("String: \"%s\"\n", node->string.value);
        break;

    case NODE_IDENTIFIER:
        printf("Identifier: %s\n", node->identifier.name);
        break;

    case NODE_BOOL:
        printf("Bool: %s\n", node->boolean.value ? "true" : "false");
        break;

    case NODE_ARRAY_LITERAL:
        printf("ArrayLiteral[%d]\n", node->array_literal.count);
        for (int i = 0; i < node->array_literal.count; i++)
        {
            print_ast(node->array_literal.elements[i], indent + 1);
        }
        break;

    case NODE_INDEX_ACCESS:
        printf("IndexAccess:\n");
        print_ast(node->index_access.array, indent + 1);
        print_ast(node->index_access.index, indent + 1);
        break;

    case NODE_UNARY_OP:
        printf("UnaryOp: %d\n", node->unary_op.op);
        print_ast(node->unary_op.operand, indent + 1);
        break;

    case NODE_BINARY_OP:
        printf("BinaryOp: %d\n", node->binary_op.op);
        print_ast(node->binary_op.left, indent + 1);
        print_ast(node->binary_op.right, indent + 1);
        break;

    case NODE_FUNCTION_CALL:
        printf("FunctionCall: %s\n", node->function_call.name);
        for (int i = 0; i < node->function_call.arg_count; i++)
        {
            print_ast(node->function_call.arguments[i], indent + 1);
        }
        break;

    case NODE_VARIABLE_DECLARATION:
        printf("VarDecl: %s %s\n", node->var_decl.type, node->var_decl.name);
        print_ast(node->var_decl.initializer, indent + 1);
        break;

    case NODE_IF_STATEMENT:
        printf("IfStatement:\n");
        for (int i = 0; i < indent; i++)
            printf("  ");
        printf("Condition:\n");
        print_ast(node->if_stmt.condition, indent + 1);
        for (int i = 0; i < indent; i++)
            printf("  ");
        printf("Then:\n");
        print_ast(node->if_stmt.if_body, indent + 1);
        if (node->if_stmt.else_body)
        {
            for (int i = 0; i < indent; i++)
                printf("  ");
            printf("Else:\n");
            print_ast(node->if_stmt.else_body, indent + 1);
        }
        break;

    case NODE_FOR_LOOP:
        printf("ForLoop:\n");
        for (int i = 0; i < indent; i++)
            printf("  ");
        printf("Init:\n");
        print_ast(node->for_loop.initializer, indent + 1);
        for (int i = 0; i < indent; i++)
            printf("  ");
        printf("Condition:\n");
        print_ast(node->for_loop.condition, indent + 1);
        for (int i = 0; i < indent; i++)
            printf("  ");
        printf("Increment:\n");
        print_ast(node->for_loop.increment, indent + 1);
        for (int i = 0; i < indent; i++)
            printf("  ");
        printf("Body:\n");
        print_ast(node->for_loop.body, indent + 1);
        break;

    case NODE_WHILE_LOOP:
        printf("WhileLoop:\n");
        print_ast(node->while_loop.condition, indent + 1);
        print_ast(node->while_loop.body, indent + 1);
        break;

    case NODE_FUNCTION_DECLARATION:
        printf("FunctionDecl: %s(", node->function_decl.name);
        for (int i = 0; i < node->function_decl.param_count; i++)
        {
            if (i > 0)
                printf(", ");
            printf("%s", node->function_decl.params[i]);
        }
        printf(")\n");
        print_ast(node->function_decl.body, indent + 1);
        break;

    case NODE_RETURN_STATEMENT:
        printf("Return:\n");
        print_ast(node->return_stmt.value, indent + 1);
        break;

    case NODE_BLOCK:
    case NODE_PROGRAM:
        printf("%s:\n", node->type == NODE_BLOCK ? "Block" : "Program");
        for (int i = 0; i < node->block.count; i++)
        {
            print_ast(node->block.statements[i], indent + 1);
        }
        break;
    }
}

// FILE READ
char *read_file(const char *filename)
{
    FILE *file;
    if (filename)
    {
        file = fopen(filename, "r");
        if (!file)
        {
            fprintf(stderr, "Error: Could not open file '%s'\n", filename);
            exit(1);
        }
    }
    else
    {
        file = stdin;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(size + 1);
    if (!buffer)
    {
        fprintf(stderr, "Error: Could not allocate memory for file contents\n");
        if (filename)
            fclose(file);
        exit(1);
    }
    size_t read_size = fread(buffer, 1, size, file);
    buffer[read_size] = '\0';

    if (filename)
        fclose(file);
    return buffer;
}

static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [options] [file.bak]\n"
            "\n"
            "  (no arguments)     start interactive REPL\n"
            "  file.bak           load file and run (or stdin if file is omitted with flags)\n"
            "\n"
            "Compiler / pipeline options:\n"
            "  --help, -h         show this help\n"
            "  --debug            print tokens, AST, and execution section markers\n"
            "  --trace            statement-level execution trace (environment snapshots)\n"
            "  --dump-symbols     print symbol table after semantic analysis\n"
            "  --dump-tac         print three-address code (TAC / IR) after semantic analysis\n"
            "  --no-run           stop after analysis and dumps; do not execute\n"
            "  --strict-types     enable stricter static typing (bool/array/object/auto)\n"
            "\n"
            "Phases: lex → parse → semantic analysis → (optional IR) → interpret\n",
            prog);
}

int main(int argc, char *argv[])
{
    char *source;
    const char *filename = NULL;
    int debug_mode = 0;
    int trace_mode = 0;
    int dump_tac = 0;
    int dump_symbols = 0;
    int no_run = 0;
    int strict_types = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--debug") == 0)
        {
            debug_mode = 1;
        }
        else if (strcmp(argv[i], "--trace") == 0)
        {
            trace_mode = 1;
        }
        else if (strcmp(argv[i], "--dump-tac") == 0)
        {
            dump_tac = 1;
        }
        else if (strcmp(argv[i], "--dump-symbols") == 0)
        {
            dump_symbols = 1;
        }
        else if (strcmp(argv[i], "--no-run") == 0)
        {
            no_run = 1;
        }
        else if (strcmp(argv[i], "--strict-types") == 0)
        {
            strict_types = 1;
        }
        else
        {
            filename = argv[i];
        }
    }

    source = read_file(filename);

    // If no filename was given, start interactive REPL instead of compiling.
    if (!filename && argc == 1)
    {
        repl_start();
        free(source);
        return 0;
    }

    if (debug_mode)
    {
        printf("=== TOKENS ===\n");
        Lexer *token_lexer = create_lexer(source);
        while (1)
        {
            Token *tok = lexer_get_next_token(token_lexer);
            print_token(tok);
            TokenType tt = tok->type;
            free(tok->value);
            free(tok);
            if (tt == TOKEN_EOF)
                break;
        }
        free_lexer(token_lexer);
        printf("=== END TOKENS ===\n");
    }

    Lexer *lexer = create_lexer(source);
    Parser *parser = create_parser(lexer);
    Node *ast = parse_program(parser);

    if (!ast)
    {
        printf("\nError: Failed to parse the program\n");
        free_parser(parser);
        free_lexer(lexer);
        free(source);
        return 1;
    }

    SemanticContext *sem_ctx = create_semantic_context(strict_types != 0);
    if (!analyze_program(sem_ctx, ast))
    {
        semantic_print_errors(sem_ctx);
        free_semantic_context(sem_ctx);
        free_node(ast);
        free_parser(parser);
        free_lexer(lexer);
        free(source);
        return 1;
    }

    if (debug_mode)
    {
        printf("=== AST ===\n");
        print_ast(ast, 0);
        printf("=== END AST ===\n");
    }

    if (dump_symbols)
    {
        printf("=== SYMBOL TABLE ===\n");
        print_symbol_table(sem_ctx->symbol_table);
        printf("=== END SYMBOL TABLE ===\n");
    }

    free_semantic_context(sem_ctx);

    if (dump_tac)
    {
        TAC *ir = ast_to_tac(ast);
        printf("=== TAC ===\n");
        if (ir)
            print_tac_list(ir);
        printf("=== END TAC ===\n");
        tac_free_list(ir);
    }

    if (no_run)
    {
        free_node(ast);
        free_parser(parser);
        free_lexer(lexer);
        free(source);
        return 0;
    }

    if (debug_mode)
    {
        printf("=== EXECUTION ===\n");
    }

    RuntimeError runtime_err;
    if (trace_mode)
    {
        printf("=== TRACE ===\n");
    }
    if (!interpret_program_with_trace(ast, &runtime_err, trace_mode))
    {
        if (runtime_err.has_error && runtime_err.message)
        {
            fprintf(stderr, "Runtime error at line %d, column %d: %s\n",
                    runtime_err.line, runtime_err.column, runtime_err.message);
            free(runtime_err.message);
        }
        free_node(ast);
        free_parser(parser);
        free_lexer(lexer);
        free(source);
        return 1;
    }
    if (trace_mode)
    {
        printf("=== END TRACE ===\n");
    }
    if (debug_mode)
    {
        printf("=== END EXECUTION ===\n");
    }

    free_node(ast);
    free_parser(parser);
    free_lexer(lexer);
    free(source);

    return 0;
}