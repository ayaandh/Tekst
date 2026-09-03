# Tekst

Tekst is an indentation-based programming language runtime written in C++.

## Build

```bash
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp lexer.cpp parser.cpp -o tekst
```

On Windows, build `tekst.exe` and copy it to `tk.exe` to use the package-manager command name.

## Run

```bash
tekst src/main.tekst
tekst --debug src/main.tekst
```

## Language features

- Variables and assignments
- Integers, floats, booleans, strings
- Lists and dictionaries
- Arithmetic and comparisons: `+ - * / % == != < <= > >=`
- Logical operators: `and`, `or`, `not`
- `if`, `elif`, `else`
- `while`
- `for x in list`
- `for x in range(...)`
- `break` and `continue`
- Functions with default arguments
- Classes and constructors
- `try`, `catch`, `throw`
- Imports
- String methods: `upper`, `lower`, `contains`, `replace`, `trim`
- List methods: `append`, `pop`, `contains`, `clear`
- Dictionary methods: `keys`, `values`, `get`
- Built-ins: `input`, `len`, `int`, `float`, `str`, `type`, `range`

## Standard library

The `lib` directory reserves the standard modules:

- `math`
- `random`
- `fs`
- `time`
- `os`

They are implemented by the native runtime and imported normally:

```tekst
import math
print(math.sqrt(25))
```

## tk package manager

`tk` is the Tekst package manager. It uses a static registry, so the registry can be hosted without a paid server.

```bash
tk init
tk new myapp
tk install http
tk install
tk remove http
tk list
tk search http
tk update
```

A project uses:

```text
tekst.toml
src/
packages/
tk.lock
```

The default registry is:

```text
https://tekst.ayaan.is-a.dev/packages
```

The registry can be a static directory containing `index.json` and package ZIP files.
