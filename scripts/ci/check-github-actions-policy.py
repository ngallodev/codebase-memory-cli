#!/usr/bin/env python3
"""Jenkins guard: allow only manual or reusable GitHub Actions workflows."""
from pathlib import Path
import sys

import yaml
from yaml.events import AliasEvent, DocumentEndEvent, DocumentStartEvent, NodeEvent
from yaml.nodes import MappingNode, ScalarNode

ROOT = Path(__file__).resolve().parents[2]
WORKFLOWS = ROOT / ".github" / "workflows"
ALLOWED = {"workflow_dispatch", "workflow_call"}


class PolicyLoader(yaml.SafeLoader):
    # GitHub Actions uses YAML 1.2, where the unquoted key `on` is a string.
    yaml_implicit_resolvers = {
        key: [(tag, regexp) for tag, regexp in resolvers
              if tag != "tag:yaml.org,2002:bool"]
        for key, resolvers in yaml.SafeLoader.yaml_implicit_resolvers.items()
    }

def inspect(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    try:
        for event in yaml.parse(text, Loader=PolicyLoader):
            if isinstance(event, AliasEvent) or (isinstance(event, NodeEvent) and event.anchor):
                raise ValueError("anchors and aliases are unsupported")
            if isinstance(event, DocumentStartEvent) and (event.explicit or event.version or event.tags):
                raise ValueError("explicit YAML documents and directives are unsupported")
            if isinstance(event, DocumentEndEvent) and event.explicit:
                raise ValueError("explicit YAML document endings are unsupported")
            if isinstance(event, NodeEvent) and event.tag:
                raise ValueError("explicit YAML tags are unsupported")
        root = yaml.compose(text, Loader=PolicyLoader)
        if not isinstance(root, MappingNode):
            raise ValueError("workflow document root must be a mapping")
        on_nodes = [value for key, value in root.value
                    if isinstance(key, ScalarNode) and key.value == "on"]
        if len(on_nodes) != 1:
            return [f"{path.name}: expected exactly one top-level 'on' key"]
        if not isinstance(on_nodes[0], MappingNode):
            return [f"{path.name}: top-level 'on' value must be a mapping"]
        trigger_keys = []
        for key, _ in on_nodes[0].value:
            if not isinstance(key, ScalarNode):
                raise ValueError("trigger event names must be scalar strings")
            if key.value in trigger_keys:
                return [f"{path.name}: duplicate trigger event {key.value!r}"]
            trigger_keys.append(key.value)
        docs = list(yaml.load_all(text, Loader=PolicyLoader))
    except (yaml.YAMLError, ValueError) as exc:
        return [f"{path.name}: unsupported or invalid YAML: {exc}"]

    if len(docs) != 1 or not isinstance(docs[0], dict):
        return [f"{path.name}: expected one YAML document with a workflow mapping"]
    workflow = docs[0]
    if "on" not in workflow or not isinstance(workflow["on"], dict) or not workflow["on"]:
        return [f"{path.name}: missing or non-mapping 'on' trigger declaration"]
    events = workflow["on"]
    if any(not isinstance(event, str) for event in events):
        return [f"{path.name}: trigger event names must be strings"]
    return [f"{path.name}: forbidden GitHub Actions trigger {event!r}"
            for event in events if event not in ALLOWED]


def main() -> int:
    errors = []
    if not WORKFLOWS.is_dir():
        errors.append("missing GitHub workflows directory")
    else:
        for path in sorted((*WORKFLOWS.glob("*.yml"), *WORKFLOWS.glob("*.yaml"))):
            errors.extend(inspect(path))
    if errors:
        for error in errors:
            print("CI policy violation:", error, file=sys.stderr)
        return 1
    print("GitHub Actions policy: manual/reusable only; Jenkins owns automatic CI")
    return 0


if __name__ == "__main__":
    sys.exit(main())
