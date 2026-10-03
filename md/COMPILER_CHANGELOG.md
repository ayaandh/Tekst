# LLVM compiler backend

## What changed

Tekst has been converted from an AST-interpreter-only layout to a native compiler frontend/backend layout while retaining the existing source syntax.

### New compiler components

- `src/codegen.h/.cpp`: lowers the Tekst AST into textual LLVM IR.
- `src/runtime.h/.cpp`: native runtime used by generated programs.
- `CMakeLists.txt`: reproducible compiler build and runtime deployment.
- `tests/compiler_basic.tk`: end-to-end compiler test.

### Compiler flow

`Tekst -> lexer -> parser -> AST -> LLVM IR -> clang/LLVM -> native executable`

The compiler emits `.ll` files and invokes `clang++` to perform LLVM code generation, optimization and native linking.

### Existing syntax retained

The frontend accepts the established indentation-based syntax, including `.tekst` / `.tk`, `fn` / `def`, `if` / `elif` / `else`, `while`, `for ... in`, functions with defaults, classes, `extends`, `self`, lists, dictionaries, indexing, arithmetic, comparisons, logical operators, built-ins, `let`, and both upper/lower-case boolean/null literals.

### Explicitly incomplete backend areas

The compiler does not silently reinterpret features it cannot lower. `try` / `catch`, `break` / `continue`, imports, and the complete interpreter-era standard library currently produce explicit backend errors where their lowering is not implemented.


## LLVM compiler v4
- Fixed parser handling of explicit Tekst import paths such as `import hello.tk`.
- Preserves dotted module/path components instead of stopping at the first `.` token.
- `from ... import ...` receives the same module-path handling.
- Removed the stale generated build directory from the distribution.
