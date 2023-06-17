# Create library for parser
add_library(parser STATIC parser.cc)

# Add include directories for parser
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})