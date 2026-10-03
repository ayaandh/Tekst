# Tekst LLVM Backend

Tekst compiles `.tk` / `.tekst` source through LLVM IR to a native executable.

## Low-level memory

The LLVM backend now supports explicit pointer and manual memory operations without replacing the existing Tekst syntax.

```tekst
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

- `&x` creates a pointer to a Tekst variable.
- `*p` dereferences a variable pointer.
- `alloc(n)` allocates `n` raw bytes on the heap.
- `free(p)` releases an allocation.
- `load_int` / `store_int` access 64-bit integers.
- `load_byte` / `store_byte` access one byte.
- `p + n` and `p - n` perform byte-based pointer arithmetic.
- Raw heap pointer arithmetic is bounds checked.

This is an explicit/manual memory layer. The high-level object/list/string runtime remains available, so existing Tekst programs continue to use the simpler model.

## Imports and local packages

The LLVM compiler supports local Tekst modules and packages without changing the existing `import` syntax.

Resolution order for `import name` is:

1. `%LOCALAPPDATA%/Tekst/packages/name`
2. project `packages/name`
3. a local `name/` package
4. local `name.tk`
5. local `lib/name.tk`
6. project `name.tk`

Package directories may contain multiple `.tk` files; `dec.tk` is loaded first, matching the existing package layout.

Examples:

```tekst
import mathpkg
print(mathpkg.square(8))
```

and:

```tekst
from mathpkg import square as sq
print(sq(9))
```

Imports are resolved at compile time and the imported Tekst source is compiled into the same LLVM module. This means the resulting executable does not interpret the imported package at runtime.
