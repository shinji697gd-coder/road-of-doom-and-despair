# Validation beta.2

## Automated

- Roadmap: 75 unique IDs, Nullscapes last, critical order preserved.
- C++ data tests: round trip, malformed schema, bounds, duplicate IDs, invalid runs, non-destructive rejection.
- Layout bounds: header, tabs, toolbar, content and footer do not overlap; tabs fit the same window at 568x320, 480x270, 800x600 and 1024x576 logical points.
- Windows Geode 5.11.2 build via Actions.

## Required in Geometry Dash 2.2081

Not executable in this environment. Do not merge main before these pass.

- Open and close ten times; browse all eight pages; VIEW opens the right level.
- Search/filter during requests; close before response; repeat Refresh; test offline.
- Check layout with textures/thumbnails and without other mods.
- Check that the information gutter does not cover native VIEW, titles, completion icons or thumbnail controls.
- Switch sections during fetch: a late response must not replace STARTPOS or PROGRESSION.
- Confirm no tool button sits outside the window and no tab overlaps the list frame.
- Add notes, favorite, goal, run and preset; restart GD and check persistence.
- Switch Overview / Runs / Startpos in the detail popup; notes persist and only the selected form is shown.
- Select, edit and delete runs and markers. Adding a marker makes its level appear in Saved Only.
- Empty STARTPOS shows NO STARTPOS SAVED and CHOOSE A LEVEL, with pagination hidden.
- Export JSON; cancel restore; restore; reject a corrupt file; verify the recovery file.
- Native progress is read only and the local best never decreases.
- Check Integrated Demonlist, BetterInfo, Happy Textures and Level Thumbnails.

## Remaining work from the design report

- STARTPOS stores percentage markers. Native capture and an isolated TEST engine remain unimplemented; this UI has no launch button.
- Native progress is read on entry, refresh, detail open and response; automatic session statistics and background synchronization remain unimplemented.
- This view has a local cache, serialized requests, response-key/generation checks and a 15-second timeout. A shared cross-mod request broker remains unimplemented.
- Existing manual DONE flags stay under their old keys and are not counted as verified GD completions.
- The existing screenshot download in the build remains; a supplied personal screenshot is needed to replace it.
- Run dates remain unimplemented.
