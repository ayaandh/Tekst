# Tekst: A Simplified App-Oriented Language Interpreter

A lightweight C++17 interpreter for a custom simplified programming language designed for building functional applications with clean, minimal syntax.

## About Tekst

Tekst is an educational interpreter that implements core programming concepts in a straightforward manner. It uses Python-inspired syntax but with a more concise, streamlined design suitable for rapid prototyping and learning.

### Core Features

- **Variables & Types**: integers, booleans, strings, lists, dictionaries, doubles
- **Control Flow**: `if`/`elif`/`else`, `while`, `for ... in ...`
- **Functions**: named functions with parameters and default arguments
- **Classes & Objects**: class definitions with inheritance via `extends`, methods, constructors
- **Error Handling**: `try`/`catch` blocks for exception handling
- **Collections**: list and dictionary literals with indexing support
- **Method Calls**: object-oriented method invocation
- **Built-ins**: `print()`, `input()`, `int()`, `str()`, `bool()`, `float()`, `len()`
- **Modules**: basic import system scaffolding

## Language Syntax

### Variables & Assignment

```tekst
x = 42
name = "Alice"
is_active = True
numbers = [1, 2, 3, 4, 5]
config = {"host": "localhost", "port": 8080}
```

### Functions

Define functions with the `fn` or `def` keyword:

```tekst
fn greet(name):
  print("Hello, " + name)

fn add(a, b):
  print(a + b)

greet("World")
add(5, 3)
```

Functions with default arguments:

```tekst
fn welcome(name, greeting = "Hello"):
  print(greeting + ", " + name)
```

### Classes & Objects

```tekst
class WashingMachine:
  def __init__(self, name):
    self.name = name
  
  def beep():
    print("Beeeeeeeeeeeeeeeeeeeeeeeeep")

WashingMachine mach
mach.beep()
```

Classes can inherit from other classes:

```tekst
class Animal:
  def __init__(self, name):
    self.name = name
  
  def speak():
    print("Some sound")

class Dog extends Animal:
  def speak():
    print("Woof!")
```

### Control Flow

**if/elif/else:**
```tekst
x = 10
if x > 15:
  print("Large")
elif x > 5:
  print("Medium")
else:
  print("Small")
```

**while loops:**
```tekst
count = 0
while count < 5:
  print(count)
  count = count + 1
```

**for loops (iterate over lists):**
```tekst
items = [1, 2, 3, 4, 5]
for item in items:
  print(item)
```

### Collections

**Lists:**
```tekst
numbers = [10, 20, 30, 40]
print(numbers[0])
```

**Dictionaries:**
```tekst
person = {"name": "Bob", "age": 30}
print(person["name"])
```

### Error Handling

```tekst
try:
  x = 1 / 0
catch error:
  print("Division by zero!")
```

### Built-in Functions

```tekst
name = input("Name: ")
age = int(input("Age: "))
print("Hello, " + name)
print(age >= 18)
print(str(age))
print(len("Tekst"))
```

### Print Statement

```tekst
print("Hello, World!")
x = 42
print(x)
```

## Build & Run

### Requirements
- GCC/G++ with C++17 support
- MinGW (on Windows)

### Building (contributors

```bash
g++ -std=c++17 -O2 -o Tekst.exe src/main.cpp src/lexer.cpp src/parser.cpp -I.
```

### Running 

#### Contributors: 
```bash
g++ -std=c++17 -O2 -o tekst src/main.cpp src/lexer.cpp src/parser.cpp -I.
./tekst script.tekst
```
#### Running Code:
``` bash
Tekst script.tekst
```
##### Note the above will work only if added to path

With debug output to see tokens and AST:
```bash
./tekst --debug script.tekst
```

## Architecture

The interpreter follows a classic pipeline:

1. **Lexer** (`src/lexer.cpp`): Tokenizes source code into a stream of tokens
2. **Parser** (`src/parser.cpp`): Builds an Abstract Syntax Tree (AST) and executes it directly
3. **Interpreter**: Evaluates the AST and manages runtime state

No compilation to bytecode or intermediate representations—the interpreter executes AST nodes directly for simplicity.

## Project Structure

```
proj/
├── src/
│   ├── main.cpp          # CLI entry point and file handling
│   ├── lexer.h/cpp       # Lexical analysis
│   ├── parser.h/cpp      # Parsing and interpretation
│   ├── main.tekst        # Sample program
│   ├── Tekst.exe            # Compiled interpreter binary
├── include/              # Reserved for headers
├── tests/                # Reserved for test files
├── LICENSE
└── README.md
```

## Example Programs

### Hello World

```tekst
print("Hello, World!")
```

### Simple Calculator

```tekst
fn add(a, b):
  print(a + b)

fn multiply(a, b):
  print(a * b)

add(10, 5)
multiply(3, 7)
```

### Working with Lists

```tekst
numbers = [1, 2, 3, 4, 5]

for num in numbers:
  print(num * 2)
```

### Working with Objects

```tekst
class Counter:
  def __init__(self, start):
    self.value = start
  
  def increment():
    self.value = self.value + 1
  
  def display():
    print(self.value)

Counter counter
counter.increment()
counter.display()
```

## Limitations & Future Directions

This interpreter implements a core subset of functionality suitable for educational purposes and simple scripts. Current limitations include:

- No lambda functions or closures (yet)
- Limited module/import implementation
- No file I/O (yet)
- No networking (yet)
- No full standard library beyond the built-ins listed above
- Single-threaded execution

Future versions may add:
- Full module system with a standard library
- File and network I/O
- More comprehensive object semantics
- Lambda expressions
- Decorators
- Compilation mode (compile to bytecode or C++)

## License

See LICENSE file for details.

## Contributing

This project is actively developed. Contributions, suggestions, and bug reports are welcome!
