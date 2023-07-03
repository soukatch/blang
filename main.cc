#include "parser/parser.h"
#include <unistd.h>
#include <vector>

static inline constexpr short PATH_SIZE_BYTES{1024};

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "usage: blang <progam name>\n";
    exit(EXIT_FAILURE);
  }

  char cwd[PATH_SIZE_BYTES];
  if (!getcwd(cwd, PATH_SIZE_BYTES)) {
    perror("");
    exit(EXIT_FAILURE);
  }

  blang::lexer l{std::string{cwd}.append("/").append(argv[1])};
  std::vector<blang::token> v;
  for (blang::token t; (t = l()).tag_ != blang::tag::eof;
       v.push_back(std::move(t)))
    ;
  v.emplace_back(blang::tag::eof);
  blang::parser p{std::move(v)};

  p();
  std::cout << p.max_next() << std::endl;
}