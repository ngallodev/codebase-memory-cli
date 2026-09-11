#!/usr/bin/env bash
set -euo pipefail
root="$(git rev-parse --show-toplevel)"
hook="$root/.git/hooks/post-commit"
mkdir -p "$(dirname "$hook")"
if [[ -e "$hook" ]]; then
  grep -Fq 'scripts/jenkins-post-commit.sh' "$hook" && exit 0
  echo "refusing to overwrite existing hook: $hook" >&2
  exit 2
fi
tmp="$(mktemp "${hook}.XXXXXX")"
trap 'rm -f "$tmp"' EXIT
cat >"$tmp" <<'EOF'
#!/usr/bin/env bash
root="$(git rev-parse --show-toplevel)" || exit 0
nohup "$root/scripts/jenkins-post-commit.sh" >>"$root/.git/jenkins-post-commit.log" 2>&1 &
EOF
chmod 700 "$tmp"
mv "$tmp" "$hook"
trap - EXIT
echo "installed local Jenkins post-commit hook: $hook"
