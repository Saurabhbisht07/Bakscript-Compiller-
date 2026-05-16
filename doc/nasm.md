# NASM x86-64 Assembly Guide

## What is x86-64 Assembly?

x86-64 assembly (also known as AMD64 or Intel 64) is a low-level programming language that directly corresponds to machine code instructions for 64-bit x86 processors. It's the human-readable representation of the binary instructions that the CPU actually executes.

Key characteristics:
- **Direct hardware control**: You work directly with CPU registers, memory addresses, and processor instructions
- **No abstractions**: Unlike high-level languages, there's no compiler magic - every instruction maps directly to hardware operations
- **Platform-specific**: Code is tied to the specific processor architecture (x86-64 in this case)
- **Maximum performance**: When optimized properly, assembly can achieve the absolute best performance possible

## Why x86-64 Assembly and Not Others?

### Why Assembly at All?
- **Performance-critical applications**: Operating system kernels, device drivers, embedded systems
- **Learning computer architecture**: Understanding how computers actually work under the hood
- **Optimization**: Fine-tuning critical code sections in larger applications
- **Reverse engineering**: Understanding and analyzing existing binary programs
- **Security research**: Exploit development and vulnerability analysis

### Why x86-64 Specifically?
- **Ubiquity**: Most desktop and server processors use x86-64 architecture
- **Mature ecosystem**: Extensive documentation, tools, and community support
- **Backward compatibility**: Can run 32-bit x86 code
- **Rich instruction set**: Large variety of instructions for different operations
> **Note:** You can refer to this for the instruction set used in this project. **[Instruction Set](instruction_set.md)**

### Alternatives and Their Trade-offs:
- **ARM64**: Better for mobile/embedded, but less common on desktop/server
- **RISC-V**: Open source and growing, but still niche
- **x86-32**: Legacy, limited to 4GB memory addressing

## What is NASM?

NASM (Netwide Assembler) is a popular assembler for x86 and x86-64 architectures. An assembler translates assembly language source code into machine code object files.

### Key Features:
- **Cross-platform**: Runs on Linux, Windows, macOS, and other systems
- **Multiple output formats**: ELF, PE, COFF, Mach-O, and more
- **Macro system**: Powerful preprocessing capabilities
- **Intel syntax**: Uses the more readable Intel syntax (vs AT&T syntax)
- **Active development**: Regularly updated with new features and bug fixes

### NASM vs Other Assemblers:
- **GAS (GNU Assembler)**: Part of GNU toolchain, uses AT&T syntax by default
- **MASM (Microsoft Macro Assembler)**: Windows-specific, commercial
- **YASM**: NASM-compatible with some additional features