cmake_minimum_required(VERSION 3.26)

project(blang)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add subdirectories
add_subdirectory(lexer)
add_subdirectory(parser)

# Add the main executable and link lexer and parser libraries
add_executable(main main.cc)
target_link_libraries(main PRIVATE lexer parser)