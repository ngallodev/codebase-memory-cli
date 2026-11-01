# GitHub Actions policy — Jenkins is the CI authority

This fork deliberately delegates automatic builds, tests, lint, security audits,
soak tests, benchmarks, and release qualification to **Jenkins**.

GitHub-hosted workflows must use **only** `workflow_dispatch` (operator-initiated)
or `workflow_call` (reusable by a manually dispatched workflow) as events.
No `push`, `pull_request`, `pull_request_target`, `schedule`, `issues`,
`workflow_run`, or other automatic Actions events are allowed.

* Jenkins' `Jenkinsfile` runs `scripts/ci/check-github-actions-policy.py`.
* The GitHub release/promotion workflows are retained as **manual-only**
  fallback/publishing operations. They can consume Actions minutes when started.
* CodeQL, GitHub Pages, smoke/soak and repro workflows remain manual-only;
  GitHub Pages will **not** update automatically after merges.
* The cost-intensive PR matrix and weekly test-impact artifact producer have
  been removed. Use Jenkins for automatic PR checks and qualification evidence.
* Upstream merges may reintroduce workflows: adapt or remove their triggers
  before accepting a synchronization PR. This policy is part of the release gate.
* No GitHub Actions workflow should be used for automatic upstream synchronization.
  Monitoring may be external; any proposed integration remains a reviewable PR.

To eliminate even manual GitHub Actions usage, disable Actions in the repository's
**Settings → Actions → General**; that also disables the manual release and
Pages deployment workflows.
