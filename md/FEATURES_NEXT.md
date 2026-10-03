# Tekst Feature Direction

Tekst is designed around readable syntax, native LLVM compilation, a batteries-included standard library, safe memory APIs, excellent diagnostics, application tooling, web/server tooling, and C/C++ interoperability.

## Standard library

The standard library direction covers files, directories, processes, networking, JSON, time, text, hashing, compression, and SQLite.

## Memory safety

Tekst keeps native performance while moving common memory management into compiler/runtime ownership rules. Raw allocation remains available for low-level code with bounds checks and explicit invalid-access errors.

## Diagnostics

Compiler diagnostics use file, line, column, source context, carets, and actionable error messages. Successful builds remain silent.

## Applications

The application framework is intended to provide a simple cross-platform API for windows, controls, events, and packaging without making the language depend on one operating system.

## Web

The web direction includes HTTP clients, an HTTP server, routing, JSON request/response helpers, and native deployment.

## Interoperability

Tekst supports native C ABI integration as the stable FFI boundary and provides a path for C++ integration through compiled C++ sources and adapters.

## Language identity

Tekst aims to combine simple readable syntax, native performance, safe defaults, first-class tooling, and batteries-included application development rather than copying another language wholesale.
