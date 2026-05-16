#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/symbol_table.h"

// Simple hash function for strings
static unsigned int hash(const char *str, int size)
{
    unsigned int hash = 0;
    while (*str)
    {
        hash = (hash * 31 + *str) % size;
        str++;
    }
    return hash;
}

SymbolTable *create_symbol_table(int size)
{
    SymbolTable *table = (SymbolTable *)malloc(sizeof(SymbolTable));
    table->size = size;
    table->scope_level = 0;
    table->symbols = (Symbol **)calloc(size, sizeof(Symbol *));
    return table;
}

bool symbol_table_insert(SymbolTable *table, const char *name, SymbolType sym_type, DataType data_type)
{
    unsigned int index = hash(name, table->size);
    Symbol *current = table->symbols[index];
    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0 && current->scope_level == table->scope_level)
        {
            return false;
        }
        current = current->next;
    }
    Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));
    symbol->name = strdup(name);
    symbol->symbol_type = sym_type;
    symbol->data_type = data_type;
    symbol->is_initialized = false;
    symbol->scope_level = table->scope_level;
    symbol->next = table->symbols[index];
    table->symbols[index] = symbol;

    return true;
}

Symbol *symbol_table_lookup(SymbolTable *table, const char *name)
{
    unsigned int index = hash(name, table->size);
    Symbol *current = table->symbols[index];
    Symbol *found = NULL;
    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0 &&
            (found == NULL || current->scope_level > found->scope_level))
        {
            found = current;
        }
        current = current->next;
    }

    return found;
}

void symbol_table_enter_scope(SymbolTable *table)
{
    table->scope_level++;
}

void symbol_table_exit_scope(SymbolTable *table)
{
    for (int i = 0; i < table->size; i++)
    {
        Symbol *current = table->symbols[i];
        Symbol *prev = NULL;

        while (current != NULL)
        {
            if (current->scope_level == table->scope_level)
            {
                Symbol *to_delete = current;

                if (prev == NULL)
                {
                    table->symbols[i] = current->next;
                }
                else
                {
                    prev->next = current->next;
                }

                current = current->next;
                free(to_delete->name);
                free(to_delete);
            }
            else
            {
                prev = current;
                current = current->next;
            }
        }
    }

    table->scope_level--;
}

void symbol_table_set_initialized(SymbolTable *table, const char *name)
{
    Symbol *symbol = symbol_table_lookup(table, name);
    if (symbol)
    {
        symbol->is_initialized = true;
    }
}

void free_symbol_table(SymbolTable *table)
{
    if (!table)
        return;

    for (int i = 0; i < table->size; i++)
    {
        Symbol *current = table->symbols[i];
        while (current != NULL)
        {
            Symbol *next = current->next;
            free(current->name);
            free(current);
            current = next;
        }
    }
    free(table->symbols);
    free(table);
}

// function to print symbol table contents
void print_symbol_table(SymbolTable *table)
{
    printf("\nSymbol Table Contents:\n");
    printf("Current Scope Level: %d\n", table->scope_level);
    printf("--------------------------------------------------------\n");
    printf("%-20s %-10s %-10s %-10s %s\n",
           "Name", "Type", "DataType", "Scope", "Initialized");
    printf("--------------------------------------------------------\n");

    for (int i = 0; i < table->size; i++)
    {
        Symbol *current = table->symbols[i];
        while (current != NULL)
        {
            const char *sym_type = current->symbol_type == SYMBOL_VARIABLE ? "Variable" : "Function";
            const char *data_type = "void";
            switch (current->data_type)
            {
            case TYPE_NUM:
                data_type = "num";
                break;
            case TYPE_STR:
                data_type = "str";
                break;
            case TYPE_BOOL:
                data_type = "bool";
                break;
            case TYPE_ARRAY:
                data_type = "array";
                break;
            case TYPE_OBJECT:
                data_type = "object";
                break;
            case TYPE_AUTO:
                data_type = "auto";
                break;
            case TYPE_VOID:
            default:
                data_type = "void";
                break;
            }
            printf("%-20s %-10s %-10s %-10d %s\n",
                   current->name, sym_type, data_type,
                   current->scope_level,
                   current->is_initialized ? "Yes" : "No");
            current = current->next;
        }
    }
    printf("--------------------------------------------------------\n");
}