#include <stdio.h>
#include <string.h>

#include "ma.h"
#include "str.h"
#include "parser.h"

int main(void) {
    ma_initialize();

    char file_name[1024];
    const char* example_file_name = "example.grm";
    printf("Enter a grammar specification file name "
	   "(or something starting with @ for %s):\n", example_file_name);
    fgets(file_name, 1024, stdin);
    str_remove_trailing_newline_(file_name);
    if (file_name[0] == '@') {
	memcpy(file_name, example_file_name, strlen(example_file_name));
	file_name[strlen(example_file_name)] = 0;
    }
    printf("\n");

    char* grammar_spec = str_read_file(file_name);

    if (grammar_spec == NULL) {
	printf("There was an error reading the file.\n");
	goto end_label;
    }

    char diagnostic[2048];
    prGrammar* grammar = prGrammar_create_from_spec(grammar_spec, diagnostic);
    FREE(grammar_spec);
    if (grammar == NULL) {
	printf("There was an error forming the grammar "
	       "based on the file's contents.\n%s", diagnostic);
	goto end_label;
    }

    prGrammar_print(grammar);

    prSLRParser* parser =
	prSLRParser_create_from_grammar(grammar, diagnostic);
    if (parser == NULL) {
	printf("%s", diagnostic);
    } else {
	printf("The grammar is SLR.\n");
    }
    
    prSLRParser_destroy(parser);
    prGrammar_destroy(grammar);
    end_label:;
    ma_finalize();
    return 0;
}
