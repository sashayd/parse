This is a C library for SLR parsing. I hope to add more comments and more features in the future. Remarks/suggestions/bug finds are welcome.

## APIs

The main APIs are:

- `./include/parser.h`: Creating a grammar from a specification, creating an SLR parser from a grammar (if it is an SLR grammar - this is discovered on the way), parsing a sequence of terminal tokens (yielding a sequence of either terminal tokens or productions) and synthesizing attributes. Note: I made up the (extermely simple) specification language, I hope it can be figured out from `./test/example.grm`.
- `./include/regex.h`: Creating an SLR parser of regex expressions and "compiling" regexes using such a parser to DFAs. Note: I made up the regex language, I hope it can be figured out from `./src/regex.c`.
- `./include/lexer.h`: Creating a lexical analyzer from a specification and processing strings into token seqeunces using such a lexical analyzer. Note: I made up an (extermely simple) specification language, I hope it can be figured out from `./test/example.lex`.

As a "backend", one has
- `./src/fa.h`: Dealing with NFAs and DFAs, as well as "DFAs with back-tracking", which is my slight abstraction of what happens in the LR parsing process.

## Tests

One can run tests using the following Linux commands ran when in terminal at root directory:
- `source 1_fa_test.sh`
- `source 2_grammar_test.sh`
- `source 3_regex_test.sh`
- `source 4_lexer_test.sh`
- `source calculator.sh`: A "concluding" test, using the components in order to create a simple calculator.

## Requirements

Everything was tested on Linux using the `gcc` compiler with the `-std=c99` option, and does not use any libraries except the standard library.
