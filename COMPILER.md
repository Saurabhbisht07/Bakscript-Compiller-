# BakScript compiler architecture

This document summarizes **phases** and **artifacts** in the reference driver (`bakscript`). It is meant for coursework or design discussions around programming-language implementation.

## Phase overview

| Phase | Responsibility | Main artifacts |
|--------|----------------|----------------|
| **Lexical analysis** | Scan raw characters into tokens | Token stream |
| **Syntax analysis** | Build structure from tokens | Abstract syntax tree (AST) |
| **Semantic analysis** | Types, scopes, declarations | Symbol table, diagnostic errors |
| **IR generation** | Flatten tree for backends | Three-address code (TAC) |
| **Interpretation** | Execute language semantics | Runtime values, I/O, trace |

The driver also includes optional **execution tracing** (statement steps and environment snapshots) for debugging and teaching.

## Intermediate representation (TAC)

TAC is a **linear IR**: instructions roughly of the form `result = arg1 op arg2`, plus labels, conditional branches, calls, and loads/stores. See `include/tac.h` and `print_tac()` in `src/tac.c` for the opcode set.

Typical compiler courses place TAC (or LLVM IR, quadruples, etc.) **after** semantic analysis and **before** optimization and code generation. This project’s `src/gen.c` targets assembly-style output from IR in a separate path; the CLI focuses on **printing** TAC for inspection.

## Symbol table

The symbol table (`include/symbol_table.h`) tracks **name → kind** (variable vs function), **data type** (`num` / `str` / `void`), **scope level**, and **initialization** state. It is populated during semantic analysis (`src/semantic.c`).

## Command-line interface

```text
bakscript                     # REPL
bakscript program.bak         # run file
bakscript --help              # options

--debug         # tokens, AST, execution section markers on stdout
--trace         # per-statement trace + environment
--dump-symbols  # print symbol table after semantic analysis
--dump-tac      # print TAC after semantic analysis
--no-run        # analyze / dump only; do not interpret
```

Chaining example (inspect IR without running):

```text
bakscript --dump-tac --dump-symbols --no-run tests\main\variables.bak
```

## Web UI

`python ui/server.py` — the **Visualize pipeline** action invokes the driver with `--debug`, `--trace`, `--dump-symbols`, and `--dump-tac` so the browser can show the same stages in one response.

## Further extensions (design ideas)

- **Optimization passes** on TAC (constant folding, dead code) with `--opt` / `--no-opt`.
- **Control-flow graph** (CFG) built from TAC for dataflow lectures.
- **Real code generation**: link `gen.c` output + `link/runtime.c` into a loadable executable.
- **Separate compilation** or modules (multiple `.bak` units, symbol resolution at link time).
