# The Road of Doom and Despair — beta.2

Windows / Geometry Dash 2.2081 / Geode 5.11.2.

75 original levels and native VIEW buttons are preserved. ROADMAP, STARTPOS and PROGRESSION share a compact strip under the title. Pagination and refresh stay in the footer. The surrounding window uses neutral panels with the game's fonts; texture packs can still style native level cells.

The small **i** button beside a level opens three separate pages:

- **OVERVIEW**: normal best, training status, favorite, current goal and notes. Notes save when switching pages or closing.
- **RUNS**: add a segment, select an existing row to edit it, or delete the selected run.
- **STARTPOS**: name and percentage markers. Select a row to rename/change it, use NEW to clear the editor, or DELETE to remove the selected marker.

The main STARTPOS section shows saved levels. CHOOSE A LEVEL or ALL LEVELS also lets you create the first marker. These are partial percentage markers; launching from a native start position remains unimplemented.

**TOOLS** groups export, restore and mod settings in one popup. Export writes training-backup.json in the mod save directory. Restore validates the entire file, asks for confirmation, and preserves the previous raw save before replacement. Safe Mode is reserved for the experimental engine; this build has no experimental gameplay hooks.

Progress is read from saved GD levels and loaded level objects; a lower value never reduces the local best. Old manual DONE flags remain preserved but are not counted as native completions. Search, filters and page are retained per view when switching sections. A local level-info cache, serialized requests and stale-response checks prevent late responses replacing the selected screen.

## Verification

    python scripts/check_roadmap.py
    c++ -std=c++20 tests/training_data.cpp -o training_data && ./training_data
    c++ -std=c++20 tests/layout.cpp -o layout_tests && ./layout_tests

The bounds test checks that title, tabs, search, list and footer occupy separate zones, including screen scaling. It does not render Cocos or validate texture-pack hooks. Windows builds run in GitHub Actions.

See [manual tests and remaining work](docs/BETA_TEST_MATRIX.md) and [save format](docs/SAVE_SCHEMA.md). This build does not complete the entire design report. Do not merge main before in-game validation.

## Credits

Interface/search approach adapted from Integrated Demonlist (MIT); see LICENSE-IntegratedDemonlist. The existing build downloads a Nullscapes screenshot via scripts/prepare_logo.py; the image and game belong to their respective owners.
