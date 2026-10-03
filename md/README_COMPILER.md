
# Tekst compiler

Tekst now has a native compiler pipeline:

```text
.tekst / .tk
    |
    v
Lexer
    |
    v
Parser -> AST
    |
    v
LLVM IR generator
    |
    v
clang / LLVM
    |
    v
native executable
```

The language syntax is intentionally unchanged. `.tekst` and `.tk` source files use the same syntax as the interpreter-era Tekst.

## Build

You need a C++17 compiler and LLVM/Clang (`clang++` on PATH).

### CMake

```bash
cmake -S . -B build
cmake --build build --config Release
```

The compiler executable is `tekst` (or `tekst.exe` on Windows). CMake copies `runtime.cpp` next to it.

### Direct build

```bash
g++ -std=c++17 -O2 src/main.cpp src/lexer.cpp src/parser.cpp src/codegen.cpp -Isrc -o tekst
```

Put `src/runtime.cpp` beside the resulting compiler executable.

## Compile

```bash
tekst main.tekst
tekst main.tekst -o hello
tekst main.tekst --emit-ir
tekst main.tekst --run
```

`--emit-ir` prints the generated LLVM IR and does not invoke Clang.

## Supported syntax

- variables and assignment
- integers, floating point numbers, booleans, strings and `None`
- arithmetic and comparisons
- lists and dictionaries
- indexing and assignment
- `if`, `elif`, `else`
- `while`
- `for ... in ...`
- `range(...)`
- `fn` and `def`
- default function arguments
- classes, `extends`, `__init__`, methods and `self`
- object fields
- built-ins: `print`, `input`, `int`, `str`, `bool`, `float`, `len`
- basic string interpolation such as `"Running {name}"`

The compiler keeps the source syntax rather than converting Tekst programs to Python, C++, or another source language. LLVM IR is the compiler's backend representation. `try` / `catch` remains reserved for the exception-aware backend work and is rejected with a clear compiler error for now.

## Notes

LLVM/Clang is required at compile time for native code generation and linking. The Tekst frontend itself is written in C++17.

The runtime is deliberately small and dynamically typed. It provides Tekst's object, collection, conversion and built-in operations to generated LLVM code. Memory management is currently simple allocation without a tracing garbage collector; a future release can replace the runtime allocator without changing Tekst syntax.

## Windows toolchain without Visual Studio

Tekst's Windows native compiler uses Clang with the MinGW-w64 GNU target. It does not use Visual Studio, MSBuild, `VsDevCmd.bat`, or a Developer Command Prompt.

Install a MinGW-w64 toolchain such as MSYS2 UCRT64 and make its `bin` directory available to Tekst. `g++.exe` must be discoverable on `PATH`, or set `MINGW_ROOT` to the MinGW installation root.

Tekst also needs `clang++`. Set `TEKST_CXX` if `clang++` is not on `PATH`.

On Windows the compiler automatically uses:

```text
--target=x86_64-w64-windows-gnu
```

and, when detected, the MinGW toolchain root with:

```text
--gcc-toolchain=<MinGW root>
```

The generated LLVM IR is temporary and is removed after a successful or failed native compilation. Successful compilation is silent. Compiler diagnostics are passed through directly.
