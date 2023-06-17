cmake_minimum_required(VERSION 3.26)

project(blang)

# Add subdirectories
add_subdirectory(lexer)
add_subdirectory(parser)

# Add the main executable and link lexer and parser libraries
add_executable(main main.cc)
target_link_libraries(main PRIVATE lexer parser)