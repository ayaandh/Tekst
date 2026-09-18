# Tekst

Tekst is an indentation-based programming language with a small, readable syntax and a native LLVM backend.

Tekst source files use the `.tk` extension. The compiler frontend lexes and parses Tekst directly into an AST, lowers that AST to LLVM IR, and then uses LLVM/Clang to produce a native executable.

## Compiler pipeline

```text
Tekst source (.tk)
       ↓
     Lexer
       ↓
     Parser
       ↓
      AST
       ↓
  LLVM IR backend
       ↓
   LLVM / Clang
       ↓
native executable
```

## Features

The native compiler currently supports:

- Variables and assignment
- Integers, floating-point numbers, booleans, strings, and `None`
- Arithmetic and comparison operators
- Logical `and` / `or`
- Lists, tuples, and dictionaries
- Indexing and indexed assignment
- `if`, `elif`, and `else`
- `while` loops
- `for ... in ...` loops
- `range(...)`
- List comprehensions
- `fn` and `def` functions
- Default function arguments
- Classes, inheritance with `extends`, constructors, methods, and `self`
- Object fields and attribute access
- Built-ins including `print`, `input`, `int`, `str`, `bool`, `float`, and `len`
- Basic string interpolation
- Local modules and packages
- Explicit pointers and low-level memory operations
- Native LLVM code generation

## Build

### Requirements

- A C++17 compiler
- LLVM/Clang with `clang++` available on `PATH`
- CMake 3.16 or newer for the recommended build

### CMake

```bash
cmake -S . -B build
cmake --build build --config Release
```

The compiler executable is `tekst` on Linux/macOS or `tekst.exe` on Windows.

CMake also places `runtime.cpp` and `runtime.h` beside the compiler executable.

### Direct GCC build

```bash
g++ -std=c++17 -O2 src/main.cpp src/lexer.cpp src/parser.cpp src/codegen.cpp -Isrc -o tekst
```

For a direct build, keep `src/runtime.cpp` and `src/runtime.h` beside the resulting compiler executable.

### Windows toolchain

The Windows native compiler uses Clang with the MinGW-w64 GNU target rather than Visual Studio or MSBuild.

MSYS2 UCRT64 is a supported way to provide the MinGW-w64 toolchain. Make its `bin` directory available on `PATH` so `g++.exe` can be found, or set `MINGW_ROOT` to the MinGW installation root.

If `clang++` is not on `PATH`, set `TEKST_CXX` to its path.

The compiler automatically uses the MinGW GNU target on Windows.

## Usage

Compile a Tekst program:

```bash
tekst main.tk
```

Choose an output name:

```bash
tekst main.tk -o hello
```

Generate LLVM IR without linking:

```bash
tekst main.tk --emit-ir > main.ll
```

Compile and immediately run:

```bash
tekst main.tk --run
```

Check the compiler version:

```bash
tekst --version
```

Successful native compilation is silent unless an option such as `--emit-ir` produces output.

## Syntax

Tekst uses indentation-based syntax.

### Hello World

```tk
print("Hello, World!")
```

### Variables and conditionals

```tk
age = int(input("Enter your age: "))

if age < 18:
    print("NO")
else:
    print("YES")
```

### Functions

```tk
fn add(a, b):
    return a + b

print(add(5, 3))
```

Default arguments are supported:

```tk
fn welcome(name, greeting = "Hello"):
    print(greeting + ", " + name)

welcome("Ayaan")
welcome("Ayaan", "Hi")
```

### Classes

```tk
class Counter:
    def __init__(self, start=0):
        self.value = start

    def increment(self, amount=1):
        self.value += amount
        return self.value

counter = Counter(10)

print(counter.increment())
print(counter.increment(5))
```

### Collections

```tk
numbers = [10, 20, 30]
numbers[1] = 25

print(numbers[1])

person = {"name": "Ayaan", "age": 14}
print(person["name"])
```

### Loops

```tk
for i in range(5):
    print(i)

count = 0

while count < 5:
    print(count)
    count += 1
```

### String interpolation

```tk
name = "Tekst"
print("Running {name}")
```

### Pointers and manual memory

Tekst also exposes an explicit low-level memory layer:

```tk
x = 41
p = &x

*p = 42
print(*p)

mem = alloc(8)
store_int(mem, 123456)
print(load_int(mem))

q = mem + 4
store_int(q, 99)
print(load_int(q))

free(mem)
```

`&x` creates a pointer to a variable, `*p` dereferences a pointer, and `alloc` / `free` provide explicit heap allocation. Raw pointer arithmetic and memory access are bounds checked by the runtime.

See `POINTERS.md` and `README_LLVM.md` for more details.

## Standard library

Tekst includes native standard-library modules. They are built into the runtime, so no `.tk` files are needed for them:

```tk
import math
import random
import fs
import time
import os

print(math.sqrt(144))
print(random.randint(1, 10))
fs.write("hello.txt", "Hello from Tekst")
print(fs.read("hello.txt"))
print(time.now())
print(os.cwd())
```

Available modules:

- `math`: `sqrt`, `cbrt`, `pow`, `abs`, `floor`, `ceil`, `round`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `log`, `log10`, `exp`, `min`, `max`, plus `pi`, `e`, and `tau`.
- `random`: `random`, `randint`, `randrange`, `uniform`, `choice`, and `seed`.
- `fs`: `exists`, `is_file`, `is_dir`, `read`, `write`, `append`, `mkdir`, `remove`, and `list`.
- `time`: `timestamp`, `now`, and `sleep`.
- `os`: `cwd`, `chdir`, and `env`.

Standard-library modules also support aliases and `from` imports:

```tk
import math as m
print(m.sqrt(81))

from random import randint as r
print(r(1, 6))
```

## Imports and packages

Local Tekst modules and packages are resolved at compile time.

For example:

```tk
from mathpkg import square as sq

print(sq(9))
```

The compiler can resolve local `.tk` modules, `lib/<name>.tk`, and package directories containing `.tk` files. A package's `dec.tk` file is loaded first when present.

Imported Tekst source is compiled into the same LLVM module, so it is not interpreted at runtime.

## Backend status

The LLVM backend covers the core execution model, including values, arithmetic, comparisons, logical operations, variables, functions, default arguments, conditionals, loops, collections, object fields, constructors, methods, built-ins, indexing, list comprehensions, imports, and basic string interpolation.

Explicit pointer and manual memory operations are also available through the native runtime.

Some interpreter-era functionality still requires dedicated LLVM lowering or runtime work. In particular, full exception unwinding and `try` / `catch` are not yet part of the native backend. `break` and `continue` also require dedicated lowering work.

When a feature is not supported by the native backend, the compiler reports an explicit error rather than silently changing the program's semantics.

## Project structure

```text
src/
├── main.cpp          CLI and compiler driver
├── lexer.h/cpp       indentation-aware lexer
├── parser.h/cpp      Tekst parser
├── ast.h/cpp         AST definitions
├── codegen.h/cpp     LLVM IR backend
├── runtime.h/cpp     native Tekst runtime
├── builtins.h/cpp    built-in operations
├── class.h/cpp       class support
├── function.h/cpp    function support
├── environment.h/cpp runtime environment
├── interpreter.h/cpp interpreter support
└── ...

tests/                compiler test programs
examples/             example `.tk` programs
docs/                 website documentation
contributing/         contribution documentation
download/             distribution/download assets
setup/                Windows installer assets
tk/                   Tk utility source
```

## Documentation

- `README_COMPILER.md` - native compiler details and supported syntax
- `README_LLVM.md` - LLVM backend, pointers, memory, and imports
- `POINTERS.md` - low-level pointer and memory documentation
- `FEATURES_NEXT.md` - planned language and platform direction
- `COMPILER_CHANGELOG.md` - compiler development history
- `CONTRIBUTING.md` - contribution guidelines

## License

See `LICENSE` for the repository license.
