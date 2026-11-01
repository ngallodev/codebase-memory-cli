# CI policy and ancestry guard receipt

## Workflow policy

`scripts/ci/check-github-actions-policy.py` parses every top-level
`.github/workflows/*.yml` and `*.yaml` file with pinned PyYAML 6.0.3. PyYAML is
installed from `requirements-ci.txt` before Jenkins lint runs.

Supported workflow syntax is YAML block and flow mappings. The document must
contain exactly one implicit YAML document whose root is a mapping and whose
top-level `on` value is a nonempty mapping. Quoted keys and event names are
handled as YAML strings; ordinary and alternative indentation and flow event
mappings receive the same checks. Only these event keys pass:

* `workflow_dispatch`
* `workflow_call`

Aliases, anchors, merge keys, duplicate top-level `on` keys, duplicate event
keys within `on`, explicit document starts/ends, YAML directives, explicit
tags, invalid YAML, non-mapping `on` values, and unsupported trigger names
fail closed.
Scalar `on` values such as `on: push` and block scalars fail because `on` must
be a mapping. Duplicate keys outside the trigger declaration are outside this
guard's scope.

Run locally:

```sh
scripts/ci/install-python-tools.sh
.ci-venv/bin/python scripts/ci/check-github-actions-policy.py
.ci-venv/bin/python scripts/ci/test_github_actions_policy.py
python3 scripts/ci/test_sync_ancestry.py
```

The policy self-test exercises real repository workflows and malicious and
positive fixtures for indentation, quoted keys, flow syntax, aliases/anchors,
merge keys, duplicate trigger keys, scalar forms, directives, explicit
documents, and tags.

## Upstream ancestry helper

`scripts/ci/check-sync-ancestry.py --checkpoint SHA --target SHA --merge SHA`
requires full commit IDs, resolves each exact object, proves checkpoint is an
ancestor of the pinned upstream target, and proves the checkpoint is an exact non-first parent of the integration
merge. Early batches do not need to contain the final campaign target. Git replacement refs and graft files are disabled
for these graph checks, so a synthetic replacement cannot manufacture the
ancestry claim. The helper is local/manual and does not run hosted Actions.
Jenkins runs its self-test, which constructs true and replacement-ref-faked
histories.

## Validation record

Passed locally on 2026-11-01:

```text
python3 scripts/ci/check-github-actions-policy.py
  GitHub Actions policy: manual/reusable only; Jenkins owns automatic CI
python3 scripts/ci/test_github_actions_policy.py
  Ran 4 tests; OK (real workflows plus positive/negative fixtures)
python3 scripts/ci/test_sync_ancestry.py
  Ran 6 tests; OK (early checkpoint, exact parent, replacement/graft spoof, invalid inputs)
git diff --check
  passed
```

The exact-worktree Codebase Memory CLI discovery attempt (`cbm projects
--json`; `cbm status --json`) failed with `codebase-memory-cli: secure CLI
coordination could not be created (cache-resolve)`. Git porcelain was empty
before and after this probe; source discovery continued with `rg` and direct
reads. Graph evidence is unavailable for this run. Parent should record the CLI
failure in its canonical backlog.

Jenkins remains the full integration gate and was not run locally. No hosted
Actions workflow was run.
