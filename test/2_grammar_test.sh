#!/bin/sh

clear

gcc -std=c99 -Wall -Wextra -pedantic -D TESTING_PRINTS -D MA_TRACK -D MA_DEBUG -I../include -o test ../src/standard.c ../src/err.c ../src/ma.c ../src/str.c ../src/gs.c ../src/ss.c ../src/fa.c ../src/parser.c 2_grammar_test.c &&
./test &&
rm ./test
