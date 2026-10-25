# Local storage

Geode saved value `training-v1` contains the versioned RDOD 1 payload. IDs key records; fields are native best percentage, favorite, training status, notes, runs and partial startpos markers. `goal` is a single level ID.

The JSON backup envelope has `schema: 1` and a string `payload`. Import validates the complete candidate before mutation and writes `training-before-import.txt` first. Native best is merged by maximum. Unknown IDs/goals and invalid values are rejected. Existing manual `completed-ID` keys remain untouched.

An unreadable saved value disables writes for that session. Restore a valid backup to recover. There is no supported forward-schema migration; unsupported versions are rejected, never guessed. Data remains local. Backup files are in the Geode mod save directory.
