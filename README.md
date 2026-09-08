# Tekst

Tekst is a lightweight C++17 interpreter for a simple, app-oriented programming language with Python-inspired syntax, indentation-based blocks, functions, classes, collections, exceptions, modules, and a built-in project/package workflow.

## Features

- Variables: integers, doubles, booleans, strings, lists, dictionaries, null
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparisons: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logic: `and`, `or`, `not`, `&&`, `||`, `!`
- Assignment: `=`, `+=`, `-=`, `*=`, `/=`, `%=`
- Control flow: `if`, `elif`, `else`, `while`, `for ... in ...`
- Loop control: `break`, `continue`
- Functions: `fn` and `def`, default arguments, variadic arguments, return values, closures
- Classes: constructors, fields, methods, inheritance with `extends`, method overriding
- Objects: attribute access, method calls, object instances
- Collections: list and dictionary literals, indexing, index assignment, negative indexes, slicing
- Strings: interpolation, indexing, slicing, `upper`, `lower`, `contains`, `replace`, `trim`, `split`, `join`
- Built-ins: `print`, `input`, `int`, `float`, `str`, `bool`, `len`, `sum`, `min`, `max`, `abs`, `range`
- File utilities: `read`, `write`, `exists`, `is_file`, `is_dir`, `listdir`, `cwd`
- Math utilities: `add`, `subtract`, `multiply`, `divide`, `modulo`, `pow`, `sqrt`, `log`, `log10`, `exp`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `ceil`, `floor`, `round`
- Error handling: `try`, `catch`, `throw`
- Modules: `import`, `from`, `as`
- Package management: `tk init`, `tk new`, `tk install`, `tk remove`, `tk list`, `tk search`, `tk update`
- REPL: `tk repl`
- Debug mode: `--debug`
- Helpful runtime suggestions for likely misspellings
- Line and column information for parser errors

## Syntax

### Variables

```tekst
x = 42
name = "Alice"
active = true
nothing = null
numbers = [1, 2, 3]
config = {"host": "localhost", "port": 8080}
```

### Functions

```tekst
fn greet(name):
    return "Hello " + name

fn add(a, b = 0):
    return a + b

fn total(*values):
    result = 0
    for value in values:
        result += value
    return result

print(greet("World"))
print(add(5, 3))
print(total(1, 2, 3, 4))
```

### Strings

```tekst
name = "Tekst"
print("Hello {name}")
print(name.upper())
print(name.lower())
print(name.contains("eks"))
print(name.replace("Tekst", "Language"))
print("  hello  ".trim())
print("a,b,c".split(","))
print(",".join(["a", "b", "c"]))
```

### Lists

```tekst
numbers = [10, 20, 30, 40]
print(numbers[0])
print(numbers[-1])
print(numbers[1:3])
numbers[0] += 5
numbers.append(50)
print(numbers)
```

### Dictionaries

```tekst
person = {"name": "Bob", "age": 30}
print(person["name"])
person["age"] += 1
print(person.keys())
print(person.values())
print(person.get("email", "unknown"))
```

### Control Flow

```tekst
score = 85

if score >= 90:
    print("A")
elif score >= 80:
    print("B")
else:
    print("C")

for n in range(5):
    if n == 3:
        continue
    print(n)
```

### Classes

```tekst
class Animal:
    fn init(name):
        self.name = name

    fn speak():
        return "animal"

class Dog extends Animal:
    fn speak():
        return "woof"

pet = Dog("Max")
print(pet.name)
print(pet.speak())
```

Constructors can also be written as `__init__` for compatibility with Python-style examples.

### Error Handling

```tekst
try:
    value = 10 / 0
catch error:
    print("Something went wrong")
```

### Modules

```tekst
import math
```

Modules are loaded from project, local, and package paths supported by the interpreter.

## Helpful Errors

Tekst reports parser errors with their location and provides suggestions for likely misspellings.

```text
error: expected assignment operator after variable name at line 3, column 9
```

Runtime name errors can suggest a nearby valid name:

```text
Undefined function or class: pritn
Did you mean 'print'?
```

## Build

Requirements:

- GCC or G++ with C++17 support
- MinGW on Windows

Build the interpreter:

```bash
g++ -std=c++17 -O2 -o tekst src/main.cpp src/lexer.cpp src/parser.cpp -I.
```

Run a program:

```bash
./tekst src/main.tk
```

Debug tokens and the parsed AST:

```bash
./tekst --debug src/main.tk
```

## CLI

```text
tk init
tk new <name>
tk install
tk install <package>
tk remove <package>
tk list
tk search <query>
tk update
tk run <file>
tk repl
tk version
tk help
```

## Project Structure

```text
proj/
├── src/
│   ├── main.cpp
│   ├── lexer.h
│   ├── lexer.cpp
│   ├── parser.h
│   ├── parser.cpp
│   └── main.tk
├── executables/
├── include/
├── tests/
├── tekst.toml
├── tk.lock
├── LICENSE
└── README.md
```

## Architecture

Tekst uses a direct AST interpreter pipeline:

1. The lexer converts source text into tokens.
2. The parser converts tokens into an AST.
3. The interpreter evaluates the AST directly.
4. The CLI provides file execution, debugging, project management, packages, and the REPL.

The project does not require a bytecode compiler or virtual machine.

## Example Application

```tekst
class Counter:
    fn init(start = 0):
        self.value = start

    fn increment(amount = 1):
        self.value += amount
        return self.value

counter = Counter(10)

for i in range(3):
    print(counter.increment())

name = "Tekst"
print("Running {name}")
```

## License

See `LICENSE` for details.

## Contributing

Tekst is actively developed. Contributions, suggestions, tests, and bug reports are welcome.
