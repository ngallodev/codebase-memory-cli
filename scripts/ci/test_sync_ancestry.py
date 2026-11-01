#!/usr/bin/env python3
"""Self-tests for exact checkpoint and true merge-parent ancestry checks."""
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

SCRIPT = Path(__file__).with_name("check-sync-ancestry.py")
SPEC = importlib.util.spec_from_file_location("sync_ancestry", SCRIPT)
ancestry = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ancestry)


class AncestryTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.repo = Path(self.temp.name)
        self.original_cwd = os.getcwd()
        os.chdir(self.repo)
        self.env = os.environ | {
            "GIT_AUTHOR_NAME": "CI policy test",
            "GIT_AUTHOR_EMAIL": "ci-policy@example.invalid",
            "GIT_COMMITTER_NAME": "CI policy test",
            "GIT_COMMITTER_EMAIL": "ci-policy@example.invalid",
        }
        self.git("init", "-q")
        self.git("config", "advice.graftFileDeprecated", "false")
        self.git("config", "user.name", "CI policy test")
        self.git("config", "user.email", "ci-policy@example.invalid")
        self.git("commit", "--allow-empty", "-qm", "base")
        self.base = self.git("rev-parse", "HEAD")
        self.git("checkout", "-qb", "side", self.base)
        self.git("commit", "--allow-empty", "-qm", "target")
        self.target = self.git("rev-parse", "HEAD")
        self.git("checkout", "-q", "-",)
        self.git("merge", "--no-ff", "-qm", "real merge", "side")
        self.merge = self.git("rev-parse", "HEAD")

    def tearDown(self):
        os.chdir(self.original_cwd)
        self.temp.cleanup()

    def git(self, *args):
        return subprocess.check_output(["git", *args], cwd=self.repo, env=self.env,
                                       text=True, stderr=subprocess.PIPE).strip()

    def test_accepts_real_checkpoint_and_second_parent(self):
        ancestry.verify(self.target, self.target, self.merge)

    def test_accepts_early_checkpoint_before_campaign_target(self):
        self.git("checkout", "-q", "side")
        self.git("commit", "--allow-empty", "-qm", "later campaign target")
        campaign_target = self.git("rev-parse", "HEAD")
        ancestry.verify(self.target, campaign_target, self.merge)

    def test_rejects_first_parent_or_wrong_checkpoint(self):
        with self.assertRaises(ValueError):
            ancestry.verify(self.base, self.target, self.merge)
        self.git("checkout", "-q", "side")
        self.git("commit", "--allow-empty", "-qm", "unmerged checkpoint")
        unmerged = self.git("rev-parse", "HEAD")
        with self.assertRaises(ValueError):
            ancestry.verify(unmerged, unmerged, self.merge)

    def test_rejects_ancestor_claim_using_replacement_ref(self):
        self.git("checkout", "--orphan", "unrelated")
        self.git("commit", "--allow-empty", "-qm", "unrelated target")
        unrelated = self.git("rev-parse", "HEAD")
        self.git("replace", "--graft", unrelated, self.base)
        with self.assertRaises(ValueError):
            ancestry.verify(self.base, unrelated, self.merge)

    def test_rejects_grafted_merge_parent(self):
        self.git("commit", "--allow-empty", "-qm", "ordinary commit")
        ordinary = self.git("rev-parse", "HEAD")
        graft = self.repo / ".git" / "info" / "grafts"
        graft.write_text(f"{ordinary} {self.base} {self.target}\n")
        with self.assertRaises(ValueError):
            ancestry.verify(self.target, self.target, ordinary)
        with mock.patch.dict(os.environ, {"GIT_GRAFT_FILE": str(graft)}):
            with self.assertRaises(ValueError):
                ancestry.verify(self.target, self.target, ordinary)

    def test_rejects_non_merge_result_and_abbreviated_sha(self):
        with self.assertRaises(ValueError):
            ancestry.verify(self.base, self.target, self.target)
        with self.assertRaises(ValueError):
            ancestry.verify(self.base[:12], self.target, self.merge)


if __name__ == "__main__":
    unittest.main()
