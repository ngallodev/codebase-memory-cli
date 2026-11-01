#!/usr/bin/env python3
"""Focused policy-guard tests; also validates every workflow in this checkout."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

SCRIPT = Path(__file__).with_name("check-github-actions-policy.py")
SPEC = importlib.util.spec_from_file_location("github_actions_policy", SCRIPT)
policy = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(policy)


class PolicyTests(unittest.TestCase):
    def rejects(self, text):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture.yml"
            path.write_text(text, encoding="utf-8")
            self.assertTrue(policy.inspect(path), text)

    def accepts(self, text):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture.yml"
            path.write_text(text, encoding="utf-8")
            self.assertEqual([], policy.inspect(path), text)

    def test_manual_and_reusable_block_and_flow_forms(self):
        self.accepts("name: valid\non:\n  workflow_dispatch:\n    inputs: {}\n")
        self.accepts("'on': {workflow_call: {}}\n")
        self.accepts('"on": {"workflow_dispatch": null}\n')

    def test_rejects_automatic_events_across_yaml_forms(self):
        for document in (
            "on:\n    push:\n",
            "'on':\n  'pull_request': {}\n",
            'on: {"schedule": [{cron: "* * * * *"}]}\n',
            "on: push\n",
            "on: |\n  workflow_dispatch\n",
        ):
            with self.subTest(document=document):
                self.rejects(document)

    def test_rejects_duplicate_alias_merge_document_directive_and_tag(self):
        for document in (
            "on: {workflow_dispatch: null}\non: {push: null}\n",
            "on: {workflow_dispatch: null, workflow_dispatch: null}\n",
            "on: &events {workflow_dispatch: null}\n",
            "on: *events\n",
            "on: {<<: {workflow_dispatch: null}, push: null}\n",
            "on: {<<: {workflow_dispatch: null}}\n",
            "<<: {on: {workflow_dispatch: null}}\non: {workflow_call: null}\n",
            "---\non: {workflow_dispatch: null}\n",
            "on: {workflow_dispatch: null}\n...\n",
            "%YAML 1.2\non: {workflow_dispatch: null}\n",
            "on: !!map {workflow_dispatch: null}\n",
            "on: {workflow_call: null}\njobs: {test: {if: true, if: false}}\n",
        ):
            with self.subTest(document=document):
                self.rejects(document)

    def test_real_workflows(self):
        paths = sorted((*policy.WORKFLOWS.glob("*.yml"), *policy.WORKFLOWS.glob("*.yaml")))
        self.assertTrue(paths)
        errors = [error for path in paths for error in policy.inspect(path)]
        self.assertEqual([], errors)


if __name__ == "__main__":
    unittest.main()
