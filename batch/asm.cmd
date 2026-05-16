nasm -f win64 -o x86_64.obj x86_64.asm
cl /c /Fo:runtime.obj ../link\runtime.c
link x86_64.obj runtime.obj /SUBSYSTEM:CONSOLE /ENTRY:_start kernel32.lib
x86_64.exe