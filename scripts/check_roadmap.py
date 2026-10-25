"""Static sanity checks for the Road of Doom and Despair beta.

No network, Geometry Dash installation or third-party Python packages required.
"""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src" / "level_data.hpp").read_text(encoding="utf-8")
META = json.loads((ROOT / "mod.json").read_text(encoding="utf-8"))

match = re.search(r'R"ROAD\((.*?)\)ROAD"', SOURCE, re.DOTALL)
assert match is not None, "Roadmap data block is missing"

rows = [line.strip() for line in match.group(1).splitlines() if line.strip()]
assert len(rows) == 75, f"Expected 75 levels, found {len(rows)}"

names: list[str] = []
ids: list[str] = []
for position, line in enumerate(rows, start=1):
    assert line.count("|") == 1, f"Invalid level at position {position}: {line!r}"
    name, level_id = line.split("|")
    assert name.strip(), f"Empty level name at position {position}"
    assert re.fullmatch(r"[1-9][0-9]*", level_id), (
        f"Invalid ID at position {position}: {level_id!r}"
    )
    names.append(name)
    ids.append(level_id)

assert len(set(ids)) == len(ids), "Duplicate Geometry Dash level IDs"
assert len(set(names)) == len(names), "Duplicate level names"
assert rows[-1] == "Nullscapes|109780665", "Nullscapes must be the final goal"
assert rows.index("Shardscapes|79997992") == rows.index(
    "Congregation|68668045"
) + 1, "Shardscapes must follow Congregation"
assert META["id"] == "gayan.road-of-doom", "Unexpected mod ID"
assert META["version"] == "v1.0.0-beta.3", "Unexpected beta version"
assert META["geode"] == "5.11.2", "Unexpected Geode SDK target"
assert META["gd"]["win"] == "2.2081", "Unexpected Geometry Dash target"

print(f"OK: {len(rows)} unique levels, Nullscapes last, beta metadata valid")

