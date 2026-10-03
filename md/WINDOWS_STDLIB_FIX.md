# Tekst stdlib Windows fix

This package contains the Tekst standard-library source with a portable
C++17 implementation of `time.sleep()`.

`time.sleep()` uses:
    std::this_thread::sleep_for(std::chrono::duration<double>(seconds))

It does NOT use `windows.h`, `Sleep`, `nanosleep64`, or other platform-specific
sleep APIs.

Important:
Tekst's compiler locates `runtime.cpp` beside the `tekst` executable first.
If you are using an already-installed `tekst.exe`, replace its adjacent
`runtime.cpp` with:

    tekst/src/runtime.cpp

Then run your program again.

Example:

    import fs

    fs.write("Hello.txt", "hi")
    text = fs.read("Hello.txt")
    print(text)

Expected output:

    hi
