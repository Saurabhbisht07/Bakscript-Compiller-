#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>

typedef enum
{
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION
} SymbolType;

// Static data types tracked by the semantic layer.
// New kinds (bool/array/object) are added in a backwards‑compatible way:
// existing code that only used num/str/void will keep working, and the
// stricter kinds are only enforced when --strict-types is enabled.
typedef enum
{
    TYPE_NUM,
    TYPE_STR,
    TYPE_BOOL,
    TYPE_ARRAY,
    TYPE_OBJECT,
    TYPE_VOID,
    TYPE_AUTO
} DataType;
typedef struct Symbol
{
    char *name;
    SymbolType symbol_type;
    DataType data_type;
    bool is_initialized;
    int scope_level;
    struct Symbol *next; 
} Symbol;

typedef struct SymbolTable
{
    Symbol **symbols; 
    int size;         
    int scope_level;  
} SymbolTable;

SymbolTable *create_symbol_table(int size);
void free_symbol_table(SymbolTable *table);
bool symbol_table_insert(SymbolTable *table, const char *name, SymbolType sym_type, DataType data_type);
Symbol *symbol_table_lookup(SymbolTable *table, const char *name);
void symbol_table_enter_scope(SymbolTable *table);
void symbol_table_exit_scope(SymbolTable *table);
void symbol_table_set_initialized(SymbolTable *table, const char *name);
void print_symbol_table(SymbolTable *table);

#endif 