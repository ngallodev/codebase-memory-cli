# Upstream 308-commit candidate audit

Audit date: 2026-09-12. Source: `codebase-memory-mcp` non-merge history (`git log --no-merges -308`). This inventory is deliberately broader than an immediate port queue: it records changes that appear useful to the CLI fork and can be reviewed/ported without importing the MCP frontend.

## Result

There are **150 potentially useful CLI-core candidates** after removing the explicitly integrated, represented, active, and previously categorized commits supplied by the maintainer. Statuses are preliminary:

- **clean**: likely applies to this fork with little or no semantic adaptation;
- **adapt**: shared C/CLI/daemon code, but tests, names, packaging, or fork-specific surfaces need adaptation;
- **blocked**: useful idea, but dependent on an excluded MCP/frontend or a feature intentionally absent from this fork.

No candidate below recommends `src/mcp/**`, MCP tests/adapters, OpenHands integration, Jenkins, or post-commit hooks.

## Existing port-plan completeness audit

This pass also walked the ancestry around every completed first-wave and active second-wave vertical. “Represented” means the behavior is already in the CLI fork even when the exact upstream hash is not present; “categorized” means it is tracked elsewhere and must be resolved before accepting a dependent fix.

| Vertical | Existing integrated/planned commits | Required predecessors or follow-ons | Verdict |
|---|---|---|---|
| Parser budget | `52701625` integrated | No missing source predecessor found; `e5c17de6` is an independent coverage/reporting vertical. | complete |
| LLVM CI/MSan | `557cb010`, `8134712` integrated | `288d4155`, `77063bca`, and `1d38be33` are separate later CI/toolchain pins in the candidate inventory. | complete, review follow-ons |
| Daemon startup/reliability | `33c4fd80`, `0567dc70`, `00060cec`, `ecfb9642`, `8116672a`, `df579830`, `61884951`, `6091db71` integrated | `8e70590d`, `9d4ac2b9`, `fc1b1ee7`, `3a8c0d97`, `7710f86e`, `4a29f0da`, `3e1703d7`, `1a6f80c3`, `daac6bd4`, `7f3e30e1` are later daemon follow-ons; `9846c3f1` is separately categorized and includes an MCP adapter gap. | complete predecessor set; follow-on queue valid |
| CLI uninstall/config safety | `c572ddc4`, `e5aabdf8`, `7bf366f6`, `91c63be5`, `a3c24a71`, `4fa11d1a`, `8135be28` integrated | `a530c8aa` is an already-represented help guard; `4c1b2347`, `4cafe8eb`, `902f85b0`, `1e0aac52`, `1e25d962`, `33d1ecea`, `b54998d2`, `d58962c0` are later independent CLI/client verticals. | complete; no omitted required predecessor |
| Search/pagination | No CLI port planned from MCP frontend | MCP-only pagination/search commits are explicitly excluded. `cc9c2f4c` is categorized as a CLI-neutral canonical-source scan fix and has no required MCP predecessor. | keep excluded; evaluate `cc9c2f4c` separately |
| Cypher limits/scope | `98899ba5` planned; `04ba2fa3`, `426e415f`, `63b99976` categorized | `afc948ba`, `25a7aacc`, and bounded-trail chain `8546070d` -> `0c5ffa50` -> `0d97729c` -> `cd22487c` -> `b3d31ca0` -> `58ef9f19` are distinct required-order candidates. | planned set has no missing predecessor; follow-on chains recorded |
| Pipeline budget/ordering | `699be6c2`, `77a0c7d9`, `e410c86f` planned/categorized | `d024f41e`, `396ac348` -> `e5f3aab0`, and Go-binding chain ending `349aecba` are separate verticals; no hidden prerequisite identified. | valid, do not combine unrelated fixes |
| Daemon memory/session | `21591d51`, `9846c3f1` planned | `c1b9c451` is the earlier Windows separator spelling change; `9846c3f1` should include only the CLI-neutral daemon portion and explicitly omit the MCP session adapter. | adapt with represented predecessor |
| Symlink ownership | `3c854ae2`, `8622d6a0` planned | `3a8b8e82` is categorized as the later agent-root policy follow-on; port in order after inode/opt-in semantics. | valid three-step vertical |
| CLI output/discovery | `b1100a49`, `48149cac` planned; `c25c1620`, `4a9fcd97`, `5f58e233` categorized | `b7952030` is the earlier library log default; quiet-mode changes are CLI-policy adaptations, not MCP dependencies. | adapt; preserve fork output policy |
| Complexity determinism | `e410c86f` planned | CP90 already contains equivalent complexity determinism coverage; compare semantics before porting. | likely represented; verify, then skip if equivalent |
| Coverage reporting | `e5c17de6` planned; `b04f5450`, `a9535092`, `cb167b9d` categorized | `f871178b` and `219b3482` are chain tests/terminology; include them only as evidence or reproduce equivalent tests. | no missing source predecessor; test chain has gaps |
| CI/release provenance | Existing workflow cleanup/pins integrated locally | Candidate `9427dd07` -> `8eff872d` -> `c3604402` is a separate fail-closed release vertical; dependency bumps are not source prerequisites. | review against fork workflows |

No integrated or planned vertical was found to require an unlisted MCP frontend, OpenHands, Jenkins, or hook commit. Where an upstream chain crosses an excluded commit, this artifact records the gap and requires a semantic adaptation rather than a direct cherry-pick.

## Highest-value merge groups

| Group | Commits | Touched paths | Status | Dependency notes |
|---|---|---|---|---|
| Daemon/activation safety | `8e70590d`, `9d4ac2b9`, `fc1b1ee7`, `3a8c0d97`, `7710f86e`, `4a29f0da`, `3e1703d7`, `1a6f80c3`, `daac6bd4`, `7f3e30e1` | `src/daemon/**`, `src/main.c`, daemon tests | adapt | Port in daemon-order; platform-specific tests are optional evidence, not MCP code. |
| Pipeline/Cypher/store correctness | `25a7aacc`, `c86f1ffa`, `afc948ba`, `4cbc9e1c`, `d90f98f5`, `396ac348`, `e5f3aab0`, `b3d31ca0`, `cd22487c`, `0d97729c`, `0c5ffa50`, `8546070d`, `58ef9f19`, `d024f41e` | `src/pipeline/**`, `src/cypher/**`, `src/store/**`, `src/pipeline/artifact.*`, tests | adapt | Keep each fix with its regression test; bounded-trail commits are one dependency chain. |
| Extraction/language coverage | `97517a46`, `592894a4`, `c36b4fbc`, `fd73c347`, `44caa4c3`, `a2ea711c`, `3f831936`, `8f50841a`, `cb7cb444`, `47116b8e`, `0b0d143c`, `95689b5c`, `2910e284`, `b6a22843`, `706bb2ce`, `7b72652a`, `1d6a140f`, `229b4fe1`, `0f1e65d6`, `5cabeb4b`, `43ecc098`, `6b23078c`, `dc0f8ae6`, `4b1cdf57` | `internal/cbm/**`, `src/discover/**`, `src/pipeline/**`, vendored grammars, tests | adapt | Language additions require license/checksum/manifest review and are independently selectable. |
| CLI/install/client behavior | `4c1b2347`, `4cafe8eb`, `8d46d258`, `09c66241`, `902f85b0`, `1e25d962`, `1e0aac52`, `410cd9a3`, `f518eaee`, `23dfea58`, `2bff501a`, `20ad3e5b`, `276664eb`, `33d1ecea`, `b54998d2`, `d58962c0`, `00e0cf38`, `7d4dc779`, `72ee8805`, `ee32816e` | `src/cli/**`, `src/main.c`, `install*`, package metadata, CLI tests | adapt | Review against the fork's intentionally reduced client surface; `4cafe8eb` is potentially blocked if its AGENTS policy is MCP-specific. |
| Resource/performance hardening | `57ef0a6d`, `40f2722d`, `fece72b0`, `ffc29f73`, `92812ae1`, `51770de0`, `f95fe55b`, `a4126427`, `a4c0ffbc`, `32ae20d0`, `e24f0c63`, `793716dc`, `f6e3af43`, `3491a8e8`, `bf46b91f`, `8669ba8f` | `src/foundation/**`, `src/store/**`, `src/pipeline/**`, `internal/cbm/**`, tests | adapt | Benchmark and sanitizer evidence recommended before porting performance-only changes. |
| CI/release/build integrity | `3b173288`, `77063bca`, `288d4155`, `7d896fee`, `1d38be33`, `9427dd07`, `32633bab`, `8eff872d`, `547f355e`, `18edfa00`, `98c1f8c3`, `c3604402`, `2a02d128`, `3308f360` | `.github/workflows/**`, `Makefile.cbm`, `scripts/**`, `flake*`, release contracts | clean/adapt | Apply only to this repository's workflows; do not introduce upstream links or Jenkins behavior. |

## Complete candidate inventory

The following hashes are the exact 150 candidates. The path family is given by the group above; exact paths can be verified with `git show --stat --oneline <hash>` in the MCP checkout. All source-bearing candidates are **adapt** unless listed clean in the CI/build group. Tests-only and documentation-only commits were not counted.

```text
3b173288 f4c7d201 e93db6a9 8d46d258 7e47043a 1770d68a 57ef0a6d c4e90284
e0785b35 572725e6 40f2722d 97517a46 4c1b2347 4cafe8eb 592894a4 c36b4fbc
fd73c347 44caa4c3 25a7aacc 9d4ac2b9 a2ea711c 3f831936 da61c81a 4d20a427
8f50841a c86f1ffa 8e70590d 77063bca cc00a027 17b5a432 288d4155 cb7cb444
47116b8e d6417ada 7d76b88f afc948ba fc1b1ee7 0b0d143c 4cbc9e1c fece72b0
95689b5c d90f98f5 2910e284 b6a22843 5821078e 7d896fee ee0a2e6d 706bb2ce
396ac348 e5f3aab0 7b72652a 1d6a140f fb7cd2f8 229b4fe1 09c66241 7fe2f87b
3f926997 5b96b067 11bbf0d8 0f1e65d6 7710f86e 6c74ae38 ee32816e 97ecfe7f
6c2f82c8 72ee8805 5a02822f fde1695d ffc29f73 3a8c0d97 902f85b0 032ebf53
0df990aa ee126155 0365ef94 680bc72e 48cb94f6 1e25d962 1e0aac52 c537bf0f
3e1703d7 1a6f80c3 547f355e 7583376f fa3a3ea3 410cd9a3 f518eaee 36b7b18b
23dfea58 606052a2 2bff501a 5bbc7b7c d9f3088b 4a29f0da b3d31ca0 1af49dfe
cd22487c 0d97729c 0c5ffa50 8546070d 9eba0279 5cabeb4b 43ecc098 c4224cf3
6b23078c dc0f8ae6 9427dd07 58ef9f19 4b1cdf57 92812ae1 773bb037 109299f2
0eb22f02 8eff872d d024f41e 47bd4b68 20ad3e5b 276664eb 4cd84422 33d1ecea
b54998d2 51770de0 459be8bf d918fce2 cf5eb61c f95fe55b a4126427 a4c0ffbc
32ae20d0 e700b621 e24f0c63 793716dc cffb4e37 170590bc f277034f f6e3af43
3491a8e8 bdb99d77 8669ba8f bf46b91f d58962c0 00e0cf38 7d4dc779 c3604402
329cfc3c a530c8aa 416ba994 daac6bd4 7f3e30e1
```

## Earliest-prerequisite merge order

These are the recommended verticals' contiguous orders. A later fix must not be selected by itself when an earlier commit supplies the data structure or behavior it changes.

- Bounded Cypher trails: `8546070d` -> `0c5ffa50` -> `0d97729c` -> `cd22487c` -> `b3d31ca0` -> `58ef9f19`.
- Go struct extraction/binding: `47116b8e` -> `cb7cb444` -> `cc00a027` -> `d6417ada` -> `7d76b88f` -> `349aecba` (`e07a12b6` is a test-only gap).
- Python bare calls: `0b0d143c` -> `95689b5c` -> `97517a46`.
- Swift scanner safety: `3f831936` -> `a2ea711c` -> `e8204092` (manifest-only gap; verify checksums).
- Cross-LSP resolution: `2910e284` -> `32ae20d0` -> `da61c81a`; `e24f0c63` and `793716dc` are optional later performance extensions.
- Importance scoring: `e5f3aab0` -> `396ac348`.
- Pkl support: `1d6a140f` -> `7b72652a`.
- ArkTS support/provenance: `0df990aa` -> `032ebf53` -> `2b51b7dc` (checksum-only).
- Ensemble routing: `ee126155` -> `6c2f82c8`.
- OMP client: `f518eaee` -> `410cd9a3` (`fa3a3ea3` is formatting-only).
- URL-builder route extraction: `d9f3088b` -> `5bbc7b7c` -> `606052a2`.
- CLI client selector: `1e0aac52` -> `1e25d962` -> `902f85b0`.
- Release integrity: `9427dd07` -> `8eff872d` -> `c3604402`.

Explicit gaps are excluded or categorized commits between an earliest prerequisite and a later fix. Port those gaps semantically, or mark the vertical blocked; do not cherry-pick only the terminal hash.

## Rejected / excluded accounting

- 18 first-wave integrated commits: excluded by exact hash.
- 12 active or categorized second-wave commits: excluded by exact hash.
- 5 already represented skips: excluded by exact hash.
- 18 previously categorized next-tranche commits: excluded by exact hash.
- MCP-only frontend/search/pagination/session changes: rejected regardless of subject; no `src/mcp/**`, MCP tests, or adapters are candidates.
- Tests-only, formatting-only, documentation-only, dependency-only, and empty CI retrigger commits: rejected from the 150-count queue unless part of a source change's dependency chain.
- OpenHands integration, Jenkins, and hook changes: explicitly rejected by scope.

The 150 count is an audit inventory, not permission to bulk-cherry-pick. Each group needs reverse applicability checks, dependency review, and focused CLI validation before implementation.

## Completion update — 2026-09-12

The completed/planned second-wave sources are now merged on the CLI branch:
`d3bb290c`, `c716cb7d`, and `fc9aeedc`. Do not select those source verticals
again.

The high-value vertical plan was then checked from each earliest prerequisite:

| Vertical | Source-chain result | CLI result |
|---|---|---|
| Bounded Cypher trails | `8546070d -> 0c5ffa50 -> 0d97729c -> cd22487c -> b3d31ca0 -> 58ef9f19` all already ancestors | no delta needed |
| Go struct extraction/binding | first five commits already ancestors; `349aecba` absent | merged as `e497dd0c` |
| Daemon activation/rendezvous | seven commits already ancestors; `8e70590d`, `9d4ac2b9`, `1a6f80c3` absent | merged as `6e289d5a` |

The parent integration check passed `git diff --check`, incremental production
build, and the selected pipeline/registry/import/daemon/CLI focused suites.

## Ponytail completion update — 2026-09-12

| Vertical | Upstream source | CLI disposition |
|---|---|---|
| Cypher live scope/capacity | `63b99976 -> 426e415f -> 04ba2fa3` | merged as `a7721244` |
| Registry receiver chain | `2c76563a` | merged as `e52b31cd` |
| Cohort handoff retry | `9104feb6` | already represented by `3036e195`; verified, no duplicate port |

The next Ponytail candidate is the earlier remaining Cypher correctness pair
`25a7aacc -> afc948ba`. Do not reselect the completed source hashes above.
