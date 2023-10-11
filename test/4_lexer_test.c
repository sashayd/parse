#include <stdio.h>
#include <string.h>

#include "standard.h"
#include "ma.h"
#include "str.h"
#include "regex.h"
#include "lexer.h"

int main(void) {
    ma_initialize();

    char file_name[1024];
    const char* example_file_name = "example.lex";
    printf("Enter a lexer specification file name "
           "(or something starting with @ for %s):\n", example_file_name);
    fgets(file_name, 1024, stdin);
    str_remove_trailing_newline_(file_name);

    if (file_name[0] == '@') {
        memcpy(file_name, example_file_name, strlen(example_file_name));
        file_name[strlen(example_file_name)] = 0;
    }
    printf("\n");

    char* lexer_spec = str_read_file(file_name);

    if (lexer_spec == NULL) {
        printf("There was an error reading the file.\n");
        goto end_label;
    }

    lexLexer* lexer = lexLexer_create_from_spec(lexer_spec, NULL);
    FREE(lexer_spec);
    if (lexer == NULL) {
        printf("There was an error forming the lexer "
               "based on the file's contents.\n");
        goto end_label;
    }

    lexLexer_print(lexer);

    char string[1024];
    printf("\nEnter a string for the lexer to lex:\n");
    fgets(string, 1024, stdin);
    str_remove_trailing_newline_(string);

    const char* end_pos;
    unsigned* const tokens =
        lexLexer_process(lexer, string, NULL, NULL, NULL, &end_pos);

    if (*end_pos != 0) {
        char* successful_prefix =
            str_create_copy_from_to(string, end_pos);
        printf("\nLexing was not successful; only lexed the prefix:\n%s\n\n",
               successful_prefix);
        FREE(successful_prefix);
    }
    else {
        printf("Lexing was successful.\n");
    }

    lexLexer_print_process_result(lexer, tokens, NULL);

    FREE(tokens);

    lexLexer_destroy(lexer);

    end_label:;
    ma_finalize();
    return 0;
}
