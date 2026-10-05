#!/usr/bin/env python3
"""Run memlab-drive's indexing path against the real CLI and daemon."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

repo = Path(__file__).resolve().parents[2]
binary = Path(sys.argv[1] if len(sys.argv) > 1 else repo / 'build/c/codebase-memory-cli').resolve()
with tempfile.TemporaryDirectory(prefix='cbm_memlab_test_') as tmp:
    root = Path(tmp)
    corpus = root / 'corpus'
    corpus.mkdir()
    (corpus / 'widget.py').write_text('def Widget():\n    return 1\n', encoding='utf-8')
    env = dict(os.environ, CBM_CACHE_DIR=str(root / 'cache'),
               CBM_RUNTIME_DIR=str(root / 'runtime'))
    (root / 'runtime').mkdir(mode=0o700)
    reply = subprocess.run(
        [sys.executable, str(repo / 'scripts/memlab-drive.py'), str(binary), str(corpus),
         '2', '--tool', 'projects'], env=env, capture_output=True, text=True, timeout=60,
    )
    assert reply.returncode == 0, reply.stderr
    assert reply.stdout.strip().endswith('served=2 failed=0'), reply.stdout
