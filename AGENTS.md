# AGENTS.md - tf2cfg

Qt 6.12 / QML desktop app (Windows, MinGW) for viewing and editing Team Fortress 2
`.cfg` files. Load a config or bundled preset, browse parsed commands, click a key on a
visual keyboard, build a `bind` line in a dialog, save surgically back to disk.
Assumed load: single user, local, one config at a time, no network.

## Layout

- `src/gamecfg/` - engine: config parser (`lex/`), `GameCfg`, `convars.h` (`cmd::Entry`).
  Deliberately UI-free. Parse output must match the game's tokenizer.
- `src/ui/` - Qt objects: `ConfigController` (load/save state machine), `CommandListModel`
  (parsed rows as a QAbstractListModel), `ConsoleCommandModel` (search).
- `qml/` - the whole UI: `Main.qml` (shell, menus, save dialogs), `pages/`, `dialogs/`,
  `keyboard/`, `theme/`, `components/`. Registered in `qml.qrc`. Custom Build-A-Bind
  panels live in `dialogs/panels/` and are chosen by a per-command registry
  (`customPanels` in `dialogs/BindEditorDialog.qml`); a new panel = one component file,
  one qrc entry, one registry entry (contract in each panel's header comment).
- `res/res.qrc` - resources incl. the command catalog `allcmds.csv` (~3800 rows), the
  companion bounds file `cfg/argbounds.csv` (`command,argIndex,type,min,max`; loaded by
  `gameconfig.cpp: loadArgBounds()`, consumed via the `argsSpec` model role), and
  bundled cfg presets under `:/`.
- `tests/tst_engine.cpp` - one QtTest binary covering engine + controller.
- `docs/` - see "Docs" below. `data/` holds reference dumps (`cvarlist_linux.txt`,
  `cs2_cvarlist.md`) the catalog was built from, not sample configs.

## Build, test, lint

Qt at `C:\Qt\6.12.0\mingw_64`, MinGW at `C:\Qt\Tools\mingw1310_64\bin` (prepend both to PATH).

```pwsh
# App (qmake6.exe both times, per the generated Makefile headers):
cd build      && qmake6 -o Makefile ..\tf2cfgreal.pro && mingw32-make -j8   # -> release/tf2cfgreal.exe
# Tests (separate build dir so a failing test never blocks the app):
cd build-test && qmake6 -o Makefile ..\tests\tests.pro && mingw32-make -j8   # -> release/tf2cfg_test.exe
cd build-test && .\release\tf2cfg_test.exe -o testlog.txt,txt
```

- The test suite is the gate: all cases green before every commit. It parses real files
  offline against temp paths; `QT += concurrent` and `RESOURCES += ../res/res.qrc` are
  required (tests read the catalog from the same resource the app uses).
- `qmllint` must stay at 0 errors; 28 unqualified-access warnings are tolerated
  (counted 2026-10-13, all are context-property `cfg` reads - a pragma sweep cannot
  fix those), do not add new ones. All QML files carry
  `pragma ComponentBehavior: Bound`; new files must too.
- `pwsh -File deploy.ps1 [-Config debug] [-QtRoot ...]` packages a self-contained `dist/`.
- Running the app for manual checks (from `build/release`, stderr is invisible without
  `QT_FORCE_STDERR_LOGGING`):
  ```pwsh
  $env:PATH = "C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
  $env:QT_FORCE_STDERR_LOGGING = "1"; .\tf2cfgreal.exe 2>err.txt
  ```
- **Kill a running `tf2cfgreal.exe` before linking** (`Stop-Process -Name tf2cfgreal`):
  the exe is locked while running and the link fails with "Permission denied". QML
  edits require a rebuild either way - they are qrc-embedded, not read from disk.

## Contracts (see docs/ARCHITECTURE.md)

`docs/ARCHITECTURE.md` is the single source for the live contracts: parser semantics
(the game's tokenizer is the acceptance bar), surgical save + post-save resync, model
contracts (`CommandListModel`, `ConsoleCommandModel`, `ConfigController`), QML rules,
and the numbered deviations **D1..D20** (cite these when deviating). The two hardest
rules: **the file on disk is the source of truth for lines the app does not own**, and
**lexer changes must reproduce the game's tokenizer** and keep the test suite green.

## Docs

- `docs/ARCHITECTURE.md` - read before changing behavior (see above).
- `docs/DESIGN-*.md` - designs for shipped features (save-backup, custom panels, enum
  panels, arg bounds); `docs/argbounds-verification.md` is the ledger of applied and
  still-unverified argument bounds. Update them when the feature changes, don't start
  new ones for small work.
- `docs/ROADMAP.md` - the single list of open work; do not re-raise dismissed items.
- `docs/AUDIT.md` - the current audit's findings; open ones are mirrored in the roadmap.
  The 2026-10-12 audit is retired to `docs/legacy/`.
- `docs/accessibility.md` - deferred A1-A8/W1-W8 items; priorities are W1 (Tf2Button
  focus ring) and W2 (KeyCap focus/keys).
- `docs/legacy/*.md` - dated records (audit, code review, migrations, the original
  QML-rewrite design): historical evidence and verification logs. Do not edit them; the
  live versions of their contracts and open items are in the two files above.

## Workflow

- Branch `qml-rewrite` is the working branch (`master` predates the rewrite).
- **Commit after each change** (Conventional Commits style); **push only when the user
  says "push"**.
- Never use PowerShell `Set-Content`/`(Get-Content) | Set-Content` on tracked files - it
  rewrites LF to CRLF. Use the edit tools, or `.Replace()` + `[IO.File]::WriteAllText`
  to strip CR if you must round-trip.
- The parser is the acceptance bar: lexer changes must reproduce the game's tokenizer
  (see `docs/ARCHITECTURE.md` section 2, verified in `docs/legacy/CODE_REVIEW.md` #1)
  and keep the test suite green. UI behavior is verified by the user manually; lint +
  build + tests are your gate.
