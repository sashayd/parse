#!/bin/sh

clear

gcc -std=c99 -Wall -Wextra -pedantic -Wno-unused-parameter -D TESTING_PRINTS -D MA_TRACK -D MA_DEBUG -I../include -o test ../src/standard.c ../src/err.c ../src/ma.c ../src/str.c ../src/gs.c ../src/ss.c ../src/fa.c ../src/parser.c ../src/regex.c ../src/lexer.c 4_lexer_test.c &&
./test &&
rm ./test
