# The Road of Doom and Despair — beta.1

Windows / Geometry Dash 2.2081 / Geode 5.11.2.

75 original levels and native VIEW buttons are preserved. Three navigation actions open ROADMAP, STARTPOS-filtered levels, and the PROGRESSION dashboard. DETAILS opens notes, favorites, current goal, training status, runs, and partial startpos markers. Use FROM / TO then ADD RUN; preset name / FROM then ADD PRESET. NEXT loads a saved preset, EDIT updates it, DELETE removes it. APPLY is deliberately locked: no native startpos engine is validated yet.

BACKUP writes training-backup.json in the mod save directory. RESTORE validates that file and retains the previous state before import. SETTINGS opens Geode settings. The Safe Mode setting is reserved for the experimental engine; no experimental gameplay hooks exist in this checkpoint.

Progress is read from loaded GD levels; a lower value never reduces the local best. Old manual DONE flags are preserved but not counted as native completions.

## Verification

`python scripts/check_roadmap.py`

`c++ -std=c++20 tests/training_data.cpp -o training_data && ./training_data`

Windows builds run in GitHub Actions. See [manual tests and remaining work](docs/BETA_TEST_MATRIX.md) and [save format](docs/SAVE_SCHEMA.md). This checkpoint is not a completed implementation of the entire design report. Do not merge main before in-game validation.

## Credits

Interface/search approach adapted from Integrated Demonlist (MIT); see LICENSE-IntegratedDemonlist. The existing build downloads a Nullscapes screenshot via scripts/prepare_logo.py; the image and game belong to their respective owners.
