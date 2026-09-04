cmake_minimum_required(VERSION 3.20)

project(Tekst LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tekst
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
)

target_include_directories(tekst PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(tk
    tk/tk.cpp
)

set_target_properties(tekst tk PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    set_target_properties(tekst tk PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG} "${CMAKE_BINARY_DIR}/bin/${CONFIG}"
    )
endforeach()
