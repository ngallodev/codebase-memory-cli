# Security hotfix receipt

Base: `ff8b8906c6807953336609cc7f464e7e4ef3cf7b`  
Branch: `sync/security-20261101`

## Changes

- Pinned `graph-ui` Vite from 6.4.3 to 6.4.5 in the manifest and lockfile.
- Pinned Vitest from 4.1.11 to 4.1.12 and the existing Monaco DOMPurify
  override from 3.4.16 to 3.4.17 after the locked dependency audit found them
  affected; both use the minimum advisory-fixed versions.
- Updated the hash-pinned PyPI publishing toolchain from certifi 2026.7.22 to
  2026.10.19 using the sdist and wheel hashes published by PyPI.
- No unrelated dependencies changed.

## Advisory and registry evidence (checked 2026-11-01)

- [GHSA-8g5g-mxqh-grgj](https://github.com/advisories/GHSA-8g5g-mxqh-grgj)
  affects Vite 6.4.4 and is fixed in 6.4.5. The advisory describes query
  parameters bypassing `server.fs.deny` for network-exposed dev servers.
- [GHSA-rq7h-c2jc-7f22](https://github.com/advisories/GHSA-rq7h-c2jc-7f22)
  and [GHSA-9jrq-w75r-8gcw](https://github.com/advisories/GHSA-9jrq-w75r-8gcw)
  list affected Vite 6.x versions through 6.4.3. The fork's starting pin was
  6.4.3; the hotfix moves it to 6.4.5.
- [GHSA-6q48-6c7w-9gw2](https://github.com/advisories/GHSA-6q48-6c7w-9gw2)
  marks certifi versions before 2026.10.18 affected and 2026.10.18 patched;
  the advisory says 2026.10.19 restores unaffected emSign roots. The publishing
  environment uses this pinned certifi build.
- [GHSA-qxg6-89mg-rqv8](https://github.com/advisories/GHSA-qxg6-89mg-rqv8)
  and [GHSA-xp26-3454-mjgp](https://github.com/advisories/GHSA-xp26-3454-mjgp)
  affect Vitest through 4.1.11; npm audit identifies 4.1.12 as the compatible
  fix.
- DOMPurify 3.4.16 is affected by
  [GHSA-cg2g-xh64-v4q7](https://github.com/advisories/GHSA-cg2g-xh64-v4q7) and
  [GHSA-mrv2-3pqr-j5jr](https://github.com/advisories/GHSA-mrv2-3pqr-j5jr);
  both advisories list 3.4.17 as fixed. The exact pin applies through the
  pre-existing Monaco override.
- `npm view vite@6.4.5 version dist.tarball dist.integrity --json` returned
  version 6.4.5, `https://registry.npmjs.org/vite/-/vite-6.4.5.tgz`, and
  integrity `sha512-4enwjFfi4IwtEh0w+jPzpLVOFQiPHmmzPxcHXQOyEur47o6u0jE573DmGpjA+7irXoMLYRZD5ZaFUEneS3abPg==`.
- PyPI JSON at `https://pypi.org/pypi/certifi/2026.10.19/json` returned the
  sdist SHA-256
  `035c1e85572b141bf1c3a3eff0e33a8eb74917522f2559075ff8d2fea780a1ae` and
  wheel SHA-256
  `42d35340cc5b9ba78e818ddbff0c5b1c2b779de448a340fd1b989f5fa8670e1b`.
- `npm view vitest@4.1.12 version dist.integrity --json` returned version
  4.1.12 and integrity
  `sha512-5eWdz2yb3ajxWF3C8C4TjkXl7FGm3n2E/VlazALrQHzK7nYMSGFnCZto0bbPNl77AwkOZVLk0YnBAejUu0NiWA==`.
- `npm view dompurify@3.4.17 version dist.integrity --json` returned version
  3.4.17 and integrity
  `sha512-B/pSlYVHQ58kqISD/yDjuJ1yfzI34m1gF4lAod2bwSRhiN4K5Cl3H8Af7hqB2PmTL911tdEudnnXn5FnhRoz0w==`.
- After updating these pins, `npm install --package-lock-only --ignore-scripts`
  reported `found 0 vulnerabilities`.

## Upstream adaptation and exclusion matrix

| Upstream item | Evidence and decision |
| --- | --- |
| #2739, merge `a3d266d705b7903552f6f232cd078ed5e0320b7e` | Its two-parent diff updates Vite 6.4.3 to 6.4.4 and certifi 2026.7.22 to 2026.10.19, with hashes. Adapted by using the later advisory-fixed Vite 6.4.5 and verified PyPI certifi hashes. |
| #2861, merge `3b46abec256049427d7523489a21a923c637f4aa` | Its diff updates Vite 6.4.4 to 6.4.5 and Vitest 4.1.11 to 4.1.12. Adapted both security-relevant updates; the live npm audit independently confirms the Vitest fixed version. |
| #2757, merge `869fc2b54ca98bf12e6f0408e68ad05d4a0468ea` | Exact merge diff touches graph-ui architecture/browser-ai source, handtest tooling, and CodeQL tests. It does not modify `src/ui/http_server.c` or `src/ui/httpd.c`. This fork serves frontend assets only through exact lookup in compiled `cbm_embedded_lookup`; request paths are matched raw, with no filesystem asset resolution. The applicable filesystem boundary is POST `/api/index`: it resolves the submitted directory with `cbm_canonical_path`, checks the resolved path against workspace authorization, stores that exact canonical path, and escapes policy errors into JSON. Existing tests cover percent-encoded route rejection, lexical `..` handling with outside-root refusal, resolved-root storage, controlled errors, and symlink-to-sensitive-root refusal (`httpd_path_match_matrix`, `ui_server_index_start_refuses_root_outside_allowed`, `ui_server_index_start_stores_resolved_root`, `ws_manifest_link_to_sensitive_directory_refused`). The separate file-picker browse endpoint intentionally lists user-selected directories and is not the asset server or indexing authorization boundary. |
| #1786, merge `8c7b3bd76e8cc0e23f5c570dafeb0ddff5c4d6fd` | Merge is not an ancestor of the fork base. Current `workspace.c` compares both slash forms as separators in Windows canonical containment and normalizes canonical Windows paths. Sensitive-root grants still require exact explicit approval. Kept current behavior; native Windows execution remains unverified. |
| #2017, merge `a3831c979547d33adaa36998dca2b004d735fc84` | Merge is not an ancestor of the fork base. Current breadth policy denies volume and too-shallow roots; there is no broad-root override. Preserved this default-deny policy; no opt-in added. |
| #2664, break-glass merge `5966b7f0` | Reviewed the merge diff: it removes compressed-size and expanded-byte ceilings in the upstream wrapper paths while retaining namespace/member checks. This fork retains finite limits in npm/PyPI/Go wrappers: 256 MiB archive, 64 members, 256 MiB per member, and 512 MiB expanded. This dependency-only PR retains those bounds. Parent integration review measured the new 13A production binary at 289 MiB, exceeding the old 256 MiB per-member limit. The integration batch must raise that finite member ceiling consistently and verify legitimate release compatibility; preserving the old limit is not evidence that larger releases install. |

## Validation and limitations

- `npm ci --ignore-scripts`, `npm run build`, and
  `npm audit --audit-level=moderate` passed after all UI pins; npm audit reports
  zero vulnerabilities. Vite emitted only its existing large-chunk advisory.
- `scripts/test.sh --suites httpd`: 96 passed, 1 skipped (Windows-only test).
- `scripts/test.sh --suites workspace`: 27 passed, including the POSIX
  symlink-to-sensitive-root case. Native Windows execution remains unverified.
- Vite, Vitest, DOMPurify manifest/lock pins and both PyPI certifi artifact
  hashes matched registry metadata.
- `npm run test:unit`: 3,813 passed, 156 failed across 14 files. Failures share
  `window.localStorage` being undefined in jsdom. The run used Node 26.9.0;
  dependency attribution and the supported-runtime baseline were not established
  by that run. Parent is investigating the runtime interaction. No test
  configuration change is included in this scoped hotfix.
- Native Windows tests were not run on this Linux host.
- Jenkins full venue gate remains pending for the parent workflow.
- Codebase Memory structural search was unavailable: `codebase-memory-cli
  projects --json` and `codebase-memory-cli status --project
  cbm-security-20261101 --json` both failed with
  `secure CLI coordination could not be created (cache-resolve)`. The failed
  probe left Git porcelain unchanged. Actionable issue for parent backlog:
  diagnose secure coordination cache resolution for this indexed worktree.
