#ifndef INTERPRETER_H
#define INTERPRETER_H

// High-level AST interpreter for BakScript.
// This walks the AST directly and executes statements/expressions without
// going through TAC/codegen, providing a fast feedback loop (REPL) and
// a reference semantics for the language.

#include "ast.h"
#include <stdbool.h>

// Runtime value kinds supported by the interpreter.
// This is intentionally slightly richer than the static type system so that
// we can model booleans explicitly while keeping backwards-compatible num/str.
typedef enum
{
    VAL_NIL,   // absence of value (e.g. no return)
    VAL_NUM,   // integer number
    VAL_STR,   // heap-allocated C string
    VAL_BOOL,  // boolean (true/false)
    VAL_ARRAY  // dynamic array of values
} ValueType;

typedef struct Value Value;

typedef struct
{
    Value *items;
    int count;
} RuntimeArray;

struct Value
{
    ValueType type;
    union
    {
        long num;
        char *str;
        int boolean;
        RuntimeArray *array;
    } as;
};

// Structured runtime error used by the interpreter.
typedef struct
{
    bool has_error;
    char *message;
    int line;
    int column;
} RuntimeError;

// Environment frame for runtime variables (and later function bindings).
typedef struct EnvEntry
{
    char *name;
    Value value;
    struct EnvEntry *next;
} EnvEntry;

typedef struct Env
{
    EnvEntry **buckets;
    int bucket_count;
    struct Env *parent; // for lexical / block scoping
} Env;

// Environment management
Env *env_create(Env *parent);
void env_free(Env *env);
bool env_define(Env *env, const char *name, Value value);
bool env_assign(Env *env, const char *name, Value value, RuntimeError *err);
bool env_get(Env *env, const char *name, Value *out, RuntimeError *err, int line, int column);

// Value helpers
Value value_num(long n);
Value value_str_owned(char *s);
Value value_str_copy(const char *s);
Value value_bool(bool b);
Value value_nil(void);
void value_free(Value *v);

// Top-level program execution.
// Returns true on success; on failure, fills err and leaves env in a valid state.
bool interpret_program(Node *program, RuntimeError *err);
bool interpret_program_with_trace(Node *program, RuntimeError *err, bool trace_enabled);

// REPL loop (reads from stdin, writes to stdout).
// This function does not return until EOF or user types "exit".
void repl_start(void);

#endif

