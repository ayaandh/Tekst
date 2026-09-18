# Tekst pointers and manual memory

Tekst keeps its existing syntax and adds low-level memory operations.

## References to variables

```tekst
x = 41
p = &x
*p = 42
print(*p)
```

`&x` creates a pointer to the variable slot and `*p` reads or writes it.

## Raw heap memory

```tekst
mem = alloc(8)
store_int(mem, 123456)
print(load_int(mem))
free(mem)
```

`alloc(n)` allocates `n` bytes. `free(p)` releases that allocation.

Raw memory is deliberately explicit: use `load_int`/`store_int` for 64-bit integers and `load_byte`/`store_byte` for bytes.

## Pointer arithmetic

```tekst
p = mem + 4
```

Pointer arithmetic is byte-based. Bounds are checked by the runtime for allocated blocks.
