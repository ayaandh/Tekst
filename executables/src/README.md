# Tekst

Tekst is an indentation-based programming language with a small, readable syntax and a native LLVM backend.

Tekst source files keep the existing `.tekst` / `.tk` syntax. The compiler frontend lexes and parses Tekst directly into an AST, lowers that AST to LLVM IR, and then uses LLVM/Clang to produce a native executable.

## Compiler pipeline

```text
Tekst source
    ↓
Lexer
    ↓
Parser
    ↓
AST
    ↓
LLVM IR backend
    ↓
LLVM/Clang
    ↓
native executable
```

## Build

Requirements:

- C++17 compiler
- LLVM/Clang with `clang++` available on PATH
- CMake 3.16+ (recommended)

### CMake

```bash
cmake -S . -B build
cmake --build build --config Release
```

The compiler is `tekst` / `tekst.exe`. CMake copies the Tekst runtime beside it.

### Direct GCC build

```bash
g++ -std=c++17 -O2 src/main.cpp src/lexer.cpp src/parser.cpp src/codegen.cpp -Isrc -o tekst
```

When using the direct build, keep `src/runtime.cpp` next to the compiler executable, or copy it there.

## Usage

Compile a program:

```bash
tekst main.tekst
```

Choose an output name:

```bash
tekst main.tekst -o hello
```

Generate LLVM IR without linking:

```bash
tekst main.tekst --emit-ir > main.ll
```

Compile and immediately run:

```bash
tekst main.tekst --run
```

Check the compiler version:

```bash
tekst --version
```

## Syntax

The compiler intentionally keeps Tekst's existing syntax. For example:

```tekst
age = int(input("enter age: "))

if age >= 18:
    print("YES")
else:
    print("NO")
```

Functions:

```tekst
fn add(a, b):
    return a + b

print(add(5, 3))
```

Default arguments:

```tekst
fn welcome(name, greeting = "Hello"):
    print(greeting + ", " + name)
```

Classes:

```tekst
class Counter:
    def __init__(self, start=0):
        self.value = start

    def increment(self, amount=1):
        self.value += amount
        return self.value

counter = Counter(10)
print(counter.increment())
```

Collections:

```tekst
numbers = [10, 20, 30]
numbers[1] = 25
print(numbers[1])

person = {"name": "Ayaan", "age": 14}
print(person["name"])
```

Loops:

```tekst
for i in range(5):
    print(i)

count = 0
while count < 5:
    print(count)
    count += 1
```

String interpolation remains supported:

```tekst
name = "Tekst"
print("Running {name}")
```

## Backend status

The LLVM backend currently covers the core execution model: values, arithmetic, comparisons, logical operations, variables, functions, default arguments, conditionals, loops, collections, object fields, constructors, methods, built-ins, indexing, and basic string interpolation.

Some interpreter-era features such as full exception unwinding, `break` / `continue` lowering, and the complete native standard library still need dedicated LLVM lowering/runtime work. Local Tekst imports and packages are now resolved at compile time. The compiler reports an explicit error for unsupported lowering instead of silently changing the language semantics.

## Project structure

```text
src/
├── main.cpp       CLI and compiler driver
├── lexer.h/cpp    indentation-aware lexer
├── parser.h/cpp   Tekst parser
├── ast.h          AST definitions
├── codegen.h/cpp  LLVM IR backend
└── runtime.h/cpp  native Tekst runtime

tests/             compiler test programs
docs/              website documentation
releases/          release pages
download/           existing distribution artifacts
```

## License

See the repository license information.
