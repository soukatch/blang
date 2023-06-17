cmake_minimum_required(VERSION 3.26)

# Create library for lexer
add_library(lexer STATIC lexer.cc)

# Add include directories for lexer
target_include_directories(lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})