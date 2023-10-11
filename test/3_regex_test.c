#include <stdio.h>
#include <string.h>

#include "standard.h"
#include "ma.h"
#include "str.h"
#include "regex.h"

int main(void) {
    ma_initialize();

    printf("Creating the regex SLR parser.\n");

    rexRegexSLRParser* regex_parser = rexRegexSLRParser_create();

    if (regex_parser == NULL) {
	printf("The regex SLR parser creation failed.\n");
	goto end_label_0;
    }

    char regex[1024];
    const char* example_regex = "\\d+(.\\d+)?";
    printf("Note: I have some non-standard regex notation, see in the source "
	   "code. Enter a regex (or something starting with @ for "
	   "%s):\n", example_regex);
    fgets(regex, 1024, stdin);
    str_remove_trailing_newline_(regex);
    if (regex[0] == '@') {
	memcpy(regex, example_regex, strlen(example_regex));
	regex[strlen(example_regex)] = 0;
    }

    rexCompiledRegex* compiled_regex =
	rexCompiledRegex_create_from_regex(regex_parser, regex, NULL);

    if (compiled_regex == NULL) {
	printf("Failed to create a DFA of chars from the regex entered "
	       "(most probably, failed to parse it).\n");
	goto end_label_1;
    }

    char string[1024];
    printf("Enter a string (for example, 0123.090):\n");
    fgets(string, 1024, stdin);
    str_remove_trailing_newline_(string);

    if (rexCompiledRegex_accepts(compiled_regex, string) == true) {
	printf("The string was accepted by the regex.\n");
    } else {
	printf("The string was not accepted by the regex.\n");
    }

    rexCompiledRegex_destroy(compiled_regex);
    end_label_1:;
    rexRegexSLRParser_destroy(regex_parser);
    end_label_0:;
    ma_finalize();
    return 0;
}
