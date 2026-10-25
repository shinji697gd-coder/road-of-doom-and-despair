# Validation beta.1

## Automated
- Roadmap: 75 unique IDs, Nullscapes last, unchanged order.
- C++ data tests: round trip, malformed schema, bounds, duplicate IDs, invalid runs, non-destructive rejection.
- Windows Geode 5.11.2 build via Actions.

## Required in Geometry Dash 2.2081
Not executable in this environment. Do not merge main before these pass.

- Open and close ten times; browse all eight pages; VIEW opens the right level.
- Search/filter during requests; close before response; repeat Refresh; test offline.
- Check layout with textures/thumbnails and without other mods.
- Add notes, favorite, goal, run and preset; restart GD and check persistence.
- Edit/delete selected preset; export JSON; restore; reject corrupted backup.
- Verify native progress is only read and local best never decreases.
- Check Integrated Demonlist, BetterInfo, Happy Textures, Level Thumbnails.

## Explicit limitations / remaining work
This is an implementation checkpoint, not full acceptance of the design report.
- STARTPOS stores percentage markers; APPLY is locked. Native capture and a validated isolated TEST engine are not implemented.
- Native progress is read when level rows load; automatic session statistics and background synchronization are not implemented.
- Request serialization protects this view; a shared cross-mod broker, timeouts and dedicated cache remain to implement.
- Existing manual DONE flags remain preserved under their old keys, but are not counted as verified GD completions.
- Existing screenshot download in the build remains; a supplied personal screenshot is needed to replace it.
- Run records can be added and viewed; editing/removal and dates remain to implement.
