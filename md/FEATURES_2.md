# Tekst 2.0 Features

Tekst supports indentation-based syntax, LLVM native compilation, optional type annotations, classes, structs, functions, collections, pointers, imports, exceptions, and control flow.

## Types

```tekst
x: int = 10

def add(a: int, b: int) -> int:
    return a + b
```

## Lambdas

```tekst
double = lambda x: x * 2
```

Lambdas currently support up to four arguments and do not capture local variables.

## Comprehensions

```tekst
squares = [x * x for x in range(10)]
```

## Pattern matching

```tekst
match value:
    case 0:
        print("zero")
    case _:
        print("other")
```

## Structs

```tekst
struct Point:
    x: int = 0
    y: int = 0

point = Point()
```

## Optional chaining

```tekst
name = user?.name
```

## Deferred expressions

```tekst
def work():
    defer cleanup()
    run()
```

## Collections

Lists provide `push`, `pop`, `remove`, `contains`, and `length`. Strings provide `upper`, `lower`, `contains`, `split`, and `replace`.

## Loop control

`break` exits a loop and `continue` proceeds to the next iteration.
