clang++ -std=c++2b lexer.cc -O3 -o lexer
cat input.txt | ./lexer > output.txt
cmp output.txt reference.txt > diff.txt
exit $?