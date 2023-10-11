#!/bin/sh

clear

gcc -std=c99 -Wall -Wextra -pedantic -Wno-unused-parameter -D MA_TRACK -D TESTING_PRINTS -D MA_DEBUG -I../include -o test ../src/standard.c ../src/err.c ../src/ma.c ../src/str.c ../src/gs.c ../src/ss.c ../src/fa.c ../src/parser.c ../src/regex.c 3_regex_test.c &&
./test &&
rm ./test
