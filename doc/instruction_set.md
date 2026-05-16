# x86_64 Assembly Instruction Reference

This document provides a comprehensive reference for all x86_64 assembly instructions used in this compiler project. Each instruction is explained with its purpose, syntax, and usage context within the generated assembly code.

## 📋 Table of Contents

- [Data Movement Instructions](#🔄-data-movement-instructions)
- [Arithmetic Instructions](#🧮-arithmetic-instructions)
- [Comparison Instructions](#🧪-comparison-instructions)
- [Conditional Set Instructions](#⚡-conditional-set-instructions)
- [Control Flow Instructions](#🔁-control-flow-instructions)
- [Function Call Instructions](#⚒️-function-call-instructions)
- [CPU State Instructions](#⚙️-cpu-state-instructions)
- [Labels and Directives](#🗃️-labels-and-directives)
- [Usage Examples](#💡-usage-examples)

---

## 🔄 Data Movement Instructions

### `mov dest, src`

**Purpose:** Copies data from source to destination  
**Syntax:** `mov destination, source`  
**Usage in Project:**

- Loading immediate values: `mov rax, 42`
- Moving between registers: `mov rcx, rax`
- Loading from memory: `mov rax, [variable]`
- Storing to memory: `mov [variable], rax`

### `lea reg, [rel label]`

**Purpose:** Loads the effective address of a memory location into a register  
**Syntax:** `lea register, [rel label]`  
**Usage in Project:**

- Loading string addresses: `lea rax, [rel string_0]`
- Position-independent code addressing with `rel` keyword

### `movzx reg, reg8`

**Purpose:** Moves an 8-bit register to a larger register with zero extension  
**Syntax:** `movzx destination_reg, source_reg8`  
**Usage in Project:**

- Converting comparison results: `movzx rax, al`
- Ensures upper bits are cleared when extending byte values

---

## 🧮 Arithmetic Instructions

### `add reg, val`

**Purpose:** Adds a value to a register  
**Syntax:** `add register, value`  
**Usage in Project:**

- Binary addition: `add rax, [variable2]`
- Implementing TAC_ADD operations

### `sub reg, val`

**Purpose:** Subtracts a value from a register  
**Syntax:** `sub register, value`  
**Usage in Project:**

- Binary subtraction: `sub rax, [variable2]`
- Implementing TAC_SUB operations

### `imul reg, val`

**Purpose:** Signed multiplication of register with value  
**Syntax:** `imul register, value`  
**Usage in Project:**

- Binary multiplication: `imul rax, [variable2]`
- Implementing TAC_MUL operations
- Result stored in the first operand

### `idiv val`

**Purpose:** Signed division of rdx:rax by the operand  
**Syntax:** `idiv operand`  
**Usage in Project:**

- Binary division: `idiv qword [variable2]`
- Quotient stored in `rax`, remainder in `rdx`
- Must be preceded by `cqo` for proper sign extension

---

## 🧪 Comparison Instructions

### `cmp op1, op2`

**Purpose:** Compares two operands by performing subtraction and setting CPU flags  
**Syntax:** `cmp operand1, operand2`  
**Usage in Project:**

- Conditional logic: `cmp rax, 0`
- Comparison operations before conditional sets
- Sets flags without modifying operands (unlike `sub`)

---

## ⚡ Conditional Set Instructions

### `setg al`

**Purpose:** Sets AL to 1 if previous comparison was "greater than", 0 otherwise  
**Usage in Project:** Implementing `TAC_GREATER` (>) operations

### `setl al`

**Purpose:** Sets AL to 1 if previous comparison was "less than", 0 otherwise  
**Usage in Project:** Implementing `TAC_LESS` (<) operations

---

## 🔁 Control Flow Instructions

### `jmp label`

**Purpose:** Unconditional jump to a labeled location  
**Syntax:** `jmp label_name`  
**Usage in Project:**

- Implementing `TAC_GOTO` operations
- Skipping else blocks: `jmp L2`

### `jne label`

**Purpose:** Jump to label if the result of last comparison was "not equal"  
**Syntax:** `jne label_name`  
**Usage in Project:**

- Implementing `TAC_IF` operations
- Jumps when condition is true (non-zero): `jne L0`

---

## ⚒️ Function Call Instructions

### `call function`

**Purpose:** Calls a procedure or external function  
**Syntax:** `call function_name`  
**Usage in Project:**

- External function calls: `call show_num`
- External function calls: `call show_str`
- Program termination: `call process_exit`

---

## ⚙️ CPU State Instructions

### `cqo`

**Purpose:** Sign-extends RAX into RDX:RAX (Convert Quad-word to Oct-word)  
**Syntax:** `cqo`  
**Usage in Project:**

- Preparing for signed division with `idiv`
- Must be called before `idiv` to properly handle negative numbers

---

## 🗃️ Labels and Directives

### `label:`

**Purpose:** Declares a target location in code for jumps and calls  
**Syntax:** `label_name:`  
**Usage in Project:**

- Control flow targets: `L0:`, `L1:`, `L2:`
- Function entry points: `_start:`

### Section Directives

- **`.data`** - Declares the data section for variables and constants
- **`.text`** - Declares the code section for executable instructions
- **`global _start`** - Makes the `_start` label globally visible as program entry point
- **`extern function_name`** - Declares external functions to be linked

---

## 💡 Usage Examples

### Variable Assignment

```assembly
; num x = 42;
mov rax, 42
mov [x], rax
```

### Arithmetic Operations

```assembly
; result = a + b;
mov rax, [a]
add rax, [b]
mov [result], rax
```

### Comparison Operations

```assembly
; t3 = score > 90;
mov rax, [score]
cmp rax, [t2]        ; t2 contains 90
setg al              ; Set AL to 1 if score > 90
movzx rax, al        ; Zero-extend to full register
mov [t3], rax        ; Store result
```

### Conditional Logic

```assembly
; if (condition) goto L0;
mov rax, [condition]
cmp rax, 0
jne L0               ; Jump to L0 if condition is true (non-zero)
```

### String Operations

```assembly
; show("Hello");
lea rcx, [rel string_0]
call show_str
```

### Function Calls

```assembly
; show(42);
mov rcx, 42
call show_num
```

---

## 📝 Notes

- **Calling Convention:** This project uses RCX for the first function argument (Windows x64 convention)
- **Position Independence:** The `rel` keyword ensures relative addressing for portable code
- **Memory Layout:** Variables are declared in the `.data` section as 64-bit values (`dq 0`)
- **Sign Extension:** Always use `cqo` before `idiv` to handle negative numbers correctly
