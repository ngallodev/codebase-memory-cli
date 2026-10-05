// Vendored tree-sitter grammar: objectscript_udl
// Each grammar compiled as separate unit (conflicting static symbols).
#include "vendored/grammars/objectscript_udl/parser.c"
/* Tree-sitter passes NULL for an empty snapshot. The scanner's byte copy is
 * needed only for nonempty snapshots; keep vendored sources unchanged. */
#define tree_sitter_objectscript_udl_external_scanner_deserialize \
    tree_sitter_objectscript_udl_external_scanner_deserialize_bytes
#include "vendored/grammars/objectscript_udl/scanner.c"
#undef tree_sitter_objectscript_udl_external_scanner_deserialize
void tree_sitter_objectscript_udl_external_scanner_deserialize(void *payload, const char *buffer,
                                                               unsigned length) {
    if (length > 0)
        tree_sitter_objectscript_udl_external_scanner_deserialize_bytes(payload, buffer, length);
}
