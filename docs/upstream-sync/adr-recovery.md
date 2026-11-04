# ADR sidecar recovery and compatibility

File-backed stores keep decision records in `<graph-db>.adr.db`. Existing
databases are compatible: the first ADR write or explicit migration copies
legacy `project_summaries` rows with `INSERT OR IGNORE`, then records the
`migrated` marker in the same sidecar transaction. A crash before commit rolls
back both copied rows and marker; retry can safely copy again. Existing sidecar
rows win conflicts. Reads do not migrate: they open an existing marked sidecar
read-only, fall back to legacy graph rows when the sidecar is absent or has no
migration marker, and otherwise return the sidecar row. `:memory:` stores
continue using the graph database.

Reindexing or replacing the graph database leaves the sidecar file untouched.
Back up and restore the graph database and its `.adr.db` sidecar together to
preserve ADRs. Stop writers and close the stores before copying the files, or
use SQLite's backup facilities for each database while coordinating writes.
Team artifacts retain the existing interchange format: export
copies the selected ADR into `project_summaries` in the snapshot, and import
restores it into the sidecar only if that project has no local ADR. A failed
artifact ADR restore is logged; graph import remains successful, so recover the
ADR from the artifact and retry after fixing the sidecar problem.

The graph row may remain after migration or deletion. The migration marker
prevents stale graph rows from being recopied after deletion; artifact export
clears snapshot summaries before injecting the current sidecar ADR, preventing
deleted rows from being resurrected. SQLite transaction recovery applies to
committed sidecar writes; copying only the graph database is not a complete
backup of post-migration ADR state.
