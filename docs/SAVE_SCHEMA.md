# Local storage

Geode saved value `training-v1` contains the versioned RDOD 1 payload. IDs key records; fields are native best percentage, favorite, training status, notes, runs and partial startpos markers. `goal` is a single level ID.

The JSON backup envelope has `schema: 1` and a string `payload`. Import validates the complete candidate before mutation and writes `training-before-import.txt` first. Native best is merged by maximum. Unknown IDs/goals and invalid values are rejected. Existing manual `completed-ID` keys remain untouched.

An unreadable saved value disables edits until a valid backup is restored. Restore retains that original raw value in the recovery file before replacement. There is no supported forward-schema migration; unsupported versions are rejected, never guessed. Data remains local. Backup files are in the Geode mod save directory.

Beta.2 keeps the same payload schema and Geode saved-value key as beta.1, so installing it preserves notes, goals, favorites, runs and markers.
