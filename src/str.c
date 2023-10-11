#include "str.h"

#include <stdio.h>
#include <string.h>
#include <standard.h>

#include "ma.h"

char* str_read_file(const char* file_name) {
    FILE* stream;
    size_t size;

    stream = fopen((const char*) file_name, "r");
    if (stream == NULL) {
	return NULL;
    }

    fseek(stream, 0, SEEK_END);
    size = ftell(stream);
    fseek(stream, 0, SEEK_SET);

    char* const result = MALLOC(size + 1);

    if (fread(result, 1, size, stream) != size) {
	FREE(result);
	return NULL;
    }
    result[size] = 0;

    fclose(stream);
    return result;
}

char* str_create_copy_from_to(const char* str_start, const char* str_end) {
    const size_t length = str_end - str_start;
    char* const result = MALLOC(length + 1);
    memcpy(result, str_start, length);
    result[length] = 0;
    return result;
}

char*	str_create_copy(const char* str) {
    const size_t length = strlen((const char*) str);
    char* const result = MALLOC(length + 1);
    memcpy(result, str, length + 1);
    return result;
}

const char* str_while_visible(const char* str) {
    const char* cursor = str;
    while (*cursor > ' ' && *cursor != 127) {
	++cursor;
    }
    return cursor;
}

const char* str_while_not_visible(const char* str) {
    const char* cursor = str;
    while (*cursor != 0 && (*cursor <= ' ' || *cursor == 127)) {
	++cursor;
    }
    return cursor;
}

const char* str_while_not_visible_except_newline(
    const char* str
    ) {
    const char* cursor = str;
    while (
	*cursor != 0 &&
	(*cursor <= ' ' || *cursor == 127) &&
	*cursor != '\n'
	) {
	++cursor;
    }
    return cursor;
}

const char* str_while_not_newline(const char* str) {
    const char* cursor = str;
    while (*cursor != 0 && *cursor != '\n') {
	++cursor;
    }
    return cursor;
}

boolean str_equal_to(const char* str, const char* comparand) {
    if (str == NULL || comparand == NULL) {
	return false;
    }
    const char* str_cursor = str;
    const char* comparand_cursor = comparand;
    for(;;) {
	if (*str_cursor != *comparand_cursor) {
	    return false;
	}
	if (*str_cursor == 0) {
	    return true;
	}
	++str_cursor;
	++comparand_cursor;
    }
    return true; /* should not really be reached */
}

size_t str_equal_to_one_of(const char* str, size_t comparand_list_length,
			   const char* const* comparand_list) {
    for (size_t i = 0; i < comparand_list_length; ++i) {
	if (str_equal_to(str, comparand_list[i]) == true) {
	    return i;
	}
    }
    return comparand_list_length;
}

const char* str_starts_with(const char* str, const char* prefix) {
    if (str == NULL || prefix == NULL) {
	return false;
    }
    const char* str_cursor = str;
    const char* prefix_cursor = prefix;
    while (*prefix_cursor != 0) {
	if (*str_cursor != *prefix_cursor) {
	    return NULL;
	}
	++str_cursor;
	++prefix_cursor;
    }
    return str_cursor;
}

const char* str_starts_with_one_of(const char* str, size_t prefix_list_length,
				   const char* const* prefix_list,
				   size_t* out_index) {
    size_t winner, winning_length;
    const char* winning_cursor = NULL;
    for (size_t i = 0; i < prefix_list_length; ++i) {
	const char* const cursor =
	    str_starts_with(str, prefix_list[i]);
	if (cursor != NULL) {
	    if (winning_cursor == NULL
		|| cursor > str + winning_length) {
		winner = i;
		winning_length = cursor - str;
		winning_cursor = cursor;
	    }
	}
    }
    if (winning_cursor != NULL) {
	*out_index = winner;
    }
    return winning_cursor;
}

void str_remove_trailing_newline_(char* str) {
    const size_t length = strlen((char*) str);
    if (length > 0 && str[length-1] == '\n') {
	str[length-1] = 0;
    }
    return;
}

char* str_goto_end(char* str) {
    for (; *str != 0; ++str) {

    }
    return str;
}
