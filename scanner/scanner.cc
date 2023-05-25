#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace blang {

/// die() - Prints an error message than exits with EXIT_FAILURE code.
/// @param s Optional message.
[[noreturn]] void die(std::string_view s = "") {
  std::cerr << s << '\n';
  exit(EXIT_FAILURE);
}

/// scan() - Scans the input stream and removes extraneous whitespace (i.e.
/// removes all but one whitespace character between non-whitespace characters);
/// maintains newlines.
/// @param input Input stream to scan.
/// @return      A stringstream result.
std::stringstream scan(std::istream &input) {
  std::stringstream ss{};

  for (int x{}; input.good() && (x = input.get()) != EOF;) {
    // once we encounter a whitespace, consume all of the successing whitespace.
    if (std::isspace(static_cast<unsigned char>(x)) != 0) {
      // keep a newline if we hit one.
      bool newline{static_cast<unsigned char>(x) == '\n'};
      for (; std::isspace(static_cast<unsigned char>(x = input.get()));)
        newline |= static_cast<unsigned char>(x) == '\n';
      ss << (newline ? '\n' : ' ');

      if (input.eof()) [[unlikely]]
        break;
    }
    ss << static_cast<unsigned char>(x);
  }
  return ss;
}
} // namespace blang

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "invalid number of arguments\n";
    exit(EXIT_FAILURE);
  }

  if (std::fstream f{argv[1], std::ios::in | std::ios::binary}; f.good())
    std::cout << blang::scan(f).rdbuf() << std::endl;
  else
    std::cerr << "could not open file for reading\n";
}