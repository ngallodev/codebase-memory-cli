"""Exercise Jenkins' actual shell steps with a mock CLI; run only in Jenkins."""

import os
from pathlib import Path
import re
import subprocess
import tempfile


pipeline = (Path(__file__).resolve().parents[1] / "Jenkinsfile").read_text()
stop = re.search(r"stage\('Stop active CBM daemon'\).*?sh '''(.*?)'''", pipeline, re.S).group(1)
restore = re.search(r"post\s*\{\s*always\s*\{\s*sh '''(.*?)'''", pipeline, re.S).group(1)

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    cli = root / ".local/bin/codebase-memory-cli"
    cli.parent.mkdir(parents=True)
    cli.write_text(
        '#!/bin/sh\n'
        'printf "%s\\n" "$*" >> "$HOME/calls"\n'
        'if [ "$2" = stop ]; then\n'
        '    test "$(cat .jenkins-installed-cli)" = "$0" || exit 9\n'
        '    exit "${MOCK_STOP_EXIT:-0}"\n'
        'fi\n'
        'if [ "$2" = start ]; then exit "${MOCK_START_EXIT:-0}"; fi\n'
    )
    cli.chmod(0o755)
    env = {**os.environ, "HOME": directory, "MOCK_START_EXIT": "0"}

    def run(script):
        return subprocess.run(["bash", "-c", script], cwd=root, env=env, check=False)

    # Recovery is recorded before stop, and a failed stop does not block the build.
    env["MOCK_STOP_EXIT"] = "7"
    assert run(stop).returncode == 0
    assert (root / ".jenkins-installed-cli").read_text().strip() == str(cli)
    # The marker survives a later build/install failure so post always can recover.
    assert run(restore).returncode == 0
    assert (root / "calls").read_text().splitlines() == ["daemon stop", "daemon start"]

    # Recovery failure must be visible to Jenkins.
    env["MOCK_START_EXIT"] = "7"
    assert run(restore).returncode == 7

    # Before the stop stage, no daemon recovery is needed.
    (root / ".jenkins-installed-cli").unlink()
    (root / "calls").unlink()
    assert run(restore).returncode == 0
    assert not (root / "calls").exists()

print("Jenkins daemon recovery checks passed")
