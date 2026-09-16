
cmake_minimum_required(VERSION 3.16)
project(Tekst LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(tekst src/main.cpp src/lexer.cpp src/parser.cpp src/codegen.cpp)
target_include_directories(tekst PRIVATE src)
add_custom_command(TARGET tekst POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime.cpp
          $<TARGET_FILE_DIR:tekst>/runtime.cpp)
add_custom_command(TARGET tekst POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime.h
          $<TARGET_FILE_DIR:tekst>/runtime.h)
install(TARGETS tekst RUNTIME DESTINATION bin)
install(FILES src/runtime.cpp src/runtime.h DESTINATION bin)
