# Linux frozen-corpus feature benchmark runbook

This runbook exercises the same feature-level benchmark contract used by the native-Windows
benchmark path before Windows RC qualification. It is intended to catch workload, corpus,
result-shape, and benchmark-harness defects on Linux without treating Linux measurements as a
substitute for the Windows baseline.

## Prerequisites

- a Linux host with `bash`, `git`, `python3`, and `sha256sum`;
- the exact `codebase-memory-cli` binary to measure, already built and executable;
- a clean checkout containing the benchmark scripts and corpus manifest;
- network access to clone/fetch the five corpus repositories during initialization.

The corpus is defined by `docs/qualification/BENCHMARK_CORPUS.json`. The five repositories are:

1. `codebase-memory-cli`;
2. `specgen-aw`;
3. `agent-workflow`;
4. `agent-workflow-spec-contracts`;
5. `herdr`.

The manifest, not this runbook, is authoritative for each repository's operations, workload file,
query, symbol, base branch, initialization ref, and eventually frozen commit.

## First run: initialize and freeze

Use a disposable benchmark workspace and results root. For an RC candidate:

```bash
scripts/qualification/run-linux-benchmark-corpus.sh \
  --binary /absolute/path/to/codebase-memory-cli \
  --workspace-root /var/tmp/cbm-linux-benchmark/repos \
  --results-root /var/tmp/cbm-linux-benchmark/results \
  --codebase-memory-ref v0.11.0-rc.1 \
  --initialize \
  --repeats 5
```

Initialization performs the following before measuring anything:

1. clones any missing corpus repository with `--no-checkout`;
2. resets `origin` to the manifest remote and fetches tags/remote refs;
3. resolves moving branch initialization refs from the fetched remote branch;
4. checks out the resolved commit detached;
5. hard-resets and cleans the benchmark checkout;
6. writes exact commit SHAs and local paths to a frozen manifest under the results root;
7. validates all frozen checkouts with `validate-corpus-checkouts.py`;
8. executes each repository's manifest-defined workload.

The codebase-memory-cli repository is special: its freeze ref must be the exact RC tag/commit that
produced the binary under measurement. Do not freeze a different source revision merely because it
is newer.

## What each repository run measures

The manifest currently selects from:

- full `index`;
- repeated `search`;
- repeated `architecture`;
- repeated `snippet`;
- repeated `outline`;
- repeated `changes`;
- repeated `status`;
- optional `cross_repo` indexing for the companion repositories.

Each repeated read operation receives one unrecorded warm-up, followed by the configured number of
measured runs. Daemon startup is kept outside steady-state read timings.

A required warm-up or measured invocation failure fails that repository. Failed invocations remain
in `timings.tsv` and are also written to `FAILURES.txt`; they are not silently removed from the
benchmark outcome.

## Evidence layout

A successful corpus run creates a timestamped directory beneath the results root:

```text
<timestamp>/
  codebase-memory-cli/
    timings.tsv
    summary.tsv
    environment.txt
    RESULT
    *.json
    *.stderr
  specgen-aw/
  agent-workflow/
  agent-workflow-spec-contracts/
  herdr/
  corpus-results.json
  RESULT
```

Every repository must contain `RESULT` with `PASS`, and the timestamp root must also contain
`RESULT=PASS`. The corpus-level marker is written only after all five expected repositories finish
successfully.

`environment.txt` records executable/harness hashes, repository commit/origin/dirty state, workload
parameters, Linux/CPU/memory/filesystem information, and active `CBM_*` environment variables.

## Repeating an already frozen corpus

Once the canonical manifest contains immutable commits and host-local paths, or when using a
previously generated frozen manifest, do not initialize again:

```bash
scripts/qualification/run-linux-benchmark-corpus.sh \
  --binary /absolute/path/to/codebase-memory-cli \
  --manifest /absolute/path/to/BENCHMARK_CORPUS.frozen.json \
  --workspace-root /var/tmp/cbm-linux-benchmark/repos \
  --results-root /var/tmp/cbm-linux-benchmark/results \
  --repeats 5
```

The runner fetches repository objects/tags but checks out the manifest's exact commit and validates
that SHA before benchmarking. A moved branch therefore cannot silently change an established
baseline generation.

## Interpreting the Linux run

For the first CLI release, Linux is a methodology/feature-parity shakeout before the Windows
`BASELINE_ZERO` capture. Use it to verify that:

- all five repositories index successfully;
- every manifest workload points at a real file/symbol surface;
- output JSON is usable by subsequent operations;
- cross-repository operations succeed where required;
- result and provenance files are complete;
- no benchmark operation is being hidden by summary filtering.

Do not compare Linux timing numbers directly to Windows RC timing gates. Hardware, filesystem,
process, and OS behavior differ. Native-Windows qualification remains authoritative for the
Windows RC baseline.
