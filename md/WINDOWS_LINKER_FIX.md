# Windows LLVM linker fix

The LLVM runtime no longer uses `std::this_thread::sleep_for()` for `time.sleep()`.
It uses `std::chrono::steady_clock` and a standard C++ busy wait instead.

This is intentionally free of `windows.h`, POSIX APIs, and `nanosleep64`.

## Important
The Tekst executable loads `runtime.cpp` from the directory containing `tekst.exe`.
After rebuilding Tekst, make sure the adjacent `runtime.cpp` is the patched one from `src/runtime.cpp`.

If you already have an old `tekst.exe`, rebuilding the compiler is required so that its copied `runtime.cpp` is replaced.
