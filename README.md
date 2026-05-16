# BakScript

BakScript is a small C-based language project with a lexer, parser, AST, semantic layer, and an interpreter runtime.

## Features

- Variables with declaration and reassignment
- Data types:
  - `num`
  - `str`
  - booleans via `true` / `false`
- Operators:
  - arithmetic: `+ - * /`
  - comparison: `== != < > <= >=`
  - logical: `&& || !`
- Control flow:
  - `when (...) { ... } otherwise { ... }`
  - `repeat (...) { ... }` (for-like loop)
  - `while (...) { ... }`
- Functions:
  - `function name(a, b) { ... }`
  - `return expr;`
- Built-ins:
  - `show(expr);`
  - `ask(prompt);`
- REPL mode (`exit` to quit)

## Build and Run

Build (Windows / MinGW):

```powershell
gcc -o bakscript src/*.c -I include
```

Run a file:

```powershell
.\bakscript.exe .\tests\main\variables.bak
```

Start REPL:

```powershell
.\bakscript.exe
```

## Language Examples

Variables:

```baks
num x = 10;
str name = "BakScript";
x = x + 1;
show("x=" + x);
show(name);
```

Control flow:

```baks
num i = 0;
while (i < 3) {
    show(i);
    i = i + 1;
}
```

Functions:

```baks
function add(a, b) {
    return a + b;
}

show(add(2, 3));
```

## Project Layout

- `src/lexer.c` / `include/lexer.h`: lexical analysis
- `src/parser.c` / `include/parser.h`: recursive-descent parser
- `src/ast.c` / `include/ast.h`: AST nodes + memory cleanup
- `src/interpreter.c` / `include/interpreter.h`: runtime execution engine
- `src/semantic.c`, `src/symbol_table.c`: semantic and symbol checks
- `src/tac.c`, `src/gen.c`, `link/runtime.c`: compiler backend and runtime linkage
- `COMPILER.md`: phases (lex → parse → semantic → TAC → run), CLI flags, extension ideas

## Compiler driver flags

After parsing, the driver runs **semantic analysis** before interpretation. Optional dumps and `--no-run`:

```powershell
c--help
.\bakscript.exe --dump-symbols --dump-tac --no-run .\tests\main\variables.bak
```

Use `--debug` and `--trace` together for the full printable pipeline (as used by the web **Visualize pipeline**).

## Sample Programs

- `tests/main/variables.bak`
- `tests/main/loops.bak`
- `tests/main/functions.bak`
- additional compatibility examples remain under `tests/main/`

## Web UI (Optional)

Run the backend server:

```powershell
python .\ui\server.py
```

Open:

```text
http://127.0.0.1:8080
```

The UI lets you:
- load sample `.bak` programs
- edit code in-browser
- run code and inspect stdout/stderr + exit code
- visualize compiler stages (tokens → AST → symbol table → TAC → execution + trace) using **Visualize Pipeline**
