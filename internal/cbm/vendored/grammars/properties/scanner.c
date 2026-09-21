#include "tree_sitter/parser.h"

#include <stdlib.h>

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

enum TokenType { FAKE_EOL };

typedef struct {
    bool reached_eof;
} Scanner;

bool tree_sitter_properties_external_scanner_scan(void *payload, TSLexer *lexer,
                                                  const bool *valid_symbols) {
    Scanner *scanner = (Scanner *)payload;
    lexer->result_symbol = FAKE_EOL;
    return scanner->reached_eof =
               !scanner->reached_eof && valid_symbols[FAKE_EOL] && lexer->eof(lexer);
}

unsigned tree_sitter_properties_external_scanner_serialize(void *payload, char *buffer) {
    return ((Scanner *)payload)->reached_eof;
}

void tree_sitter_properties_external_scanner_deserialize(void *payload, const char *buffer,
                                                         unsigned length) {
    ((Scanner *)payload)->reached_eof = length;
}

void *tree_sitter_properties_external_scanner_create() {
    return calloc(1, sizeof(Scanner));
}

void tree_sitter_properties_external_scanner_destroy(void *payload) {
    free(payload);
}
