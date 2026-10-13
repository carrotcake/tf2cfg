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
  `keyboard/`, `theme/`, `components/`. Registered in `qml.qrc`.
- `res/res.qrc` - resources incl. the command catalog `allcmds.csv` (~3800 rows) and
  bundled cfg presets under `:/`.
- `tests/tst_engine.cpp` - one QtTest binary covering engine + controller.
- `docs/` - see "Docs" below. `data/cfg/` holds sample TF2 configs.

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
- `qmllint` must stay at 0 errors; ~27 unqualified-access/ComponentBehavior warnings are
  tolerated, do not add new ones.
- `pwsh -File deploy.ps1 [-Config debug] [-QtRoot ...]` packages a self-contained `dist/`.

## Architecture contracts

- **Surgical save** (`docs/legacy/ui-qml-rewrite.md` section 10.1, implemented): the file
  on disk is the source of truth for lines the app does not own. `cmd::Entry` carries
  `sourceLine` (from the lexer's `cfgLex::currentLine()`; `-1` = created this session) and
  `appOwned` (set when this session edited the row). `writeConfig(path, baseline, error)`
  copies non-owned lines verbatim (preserving CRLF and missing final newline), re-renders
  owned lines from `args.join(" ")`, drops lines whose rows were deleted, inserts session
  rows after the last bind line, writes `.bak`, commits via `QSaveFile`. Baseline is the
  target for Save, the loaded config for Save As; empty baseline -> render everything.
- **Post-save resync**: every successful save re-parses the saved file synchronously
  (`ConfigController::resyncAfterSave`) and rebuilds rows + the `m_parsed` snapshot, so
  row bookkeeping always matches the file on disk and the saved path is the source for
  later edits. `dirty` (NOTIFY `bindsChanged`) clears until the next edit.
- **`GameCfg::appendCfg` accumulates** - always `resetParsed()` before parsing another
  file into the same instance. `loadCfg` does this.
- `CommandListModel::setBind` rebinds a parsed `bind` row in place (keeps `sourceLine`,
  sets `appOwned`); it stores the **bare command**, not a composed bind line. Head-token
  test `isBindCommand` classifies bind rows (parsed binds are typed `CMD_CMD`).
- Saving reads the model via `CommandListModel::entries()`, not `m_cfg.getCmdList()`.
- `loadCfg` parses on a worker thread (`QtConcurrent::run`); model swap happens on the GUI
  thread via `QMetaObject::invokeMethod`. Failed loads must not blank the old rows.

## QML rules

- Every QML file starts `pragma ComponentBehavior: Bound`; delegates declare
  `required property var modelData`. Keep this on new delegates or rendering breaks at
  runtime, not compile time.
- Qt 6.12 runtime: `MenuItem` has no `shortcut` property - use `Action`s for menu
  shortcuts (File menu: Ctrl+S / Ctrl+Shift+S).
- `StackView.replace()` keeps `pageStack.depth` at 1 - page identity comes from the
  explicit `editorActive` property in `Main.qml`, not depth.
- QML files are embedded in the qrc: **QML edits need an app rebuild to take effect**.
- Console output on Windows needs `QT_FORCE_STDERR_LOGGING=1` to reach stderr.

## Docs (read before changing behavior)

- `docs/legacy/ui-qml-rewrite.md` - the design: parity contract with the old widget UI
  (section 3), numbered deviations **D1..D20** (cite these when deviating), C++ API
  contract (section 6), save algorithm (section 10.1), verification records (section 12).
- `docs/AUDIT.md` - audit findings. Items marked "closed with no change" are decisions,
  not a to-do list - do not re-raise them. Currently open: #22 (CI workflow).
- `docs/CODE_REVIEW.md` - engine review; finding 1 (lexer) is fixed.
- `docs/accessibility.md` - deferred A1-A8/W1-W8 items; priorities are W1 (Tf2Button
  focus ring) and W2 (KeyCap focus/keys).
- `docs/legacy/*.md` other files are dated records - do not edit them.

## Workflow

- Branch `qml-rewrite` is the working branch (`master` predates the rewrite).
- **Commit after each change** (Conventional Commits style); **push only when the user
  says "push"**.
- Never use PowerShell `Set-Content`/`(Get-Content) | Set-Content` on tracked files - it
  rewrites LF to CRLF. Use the edit tools, or `.Replace()` + `[IO.File]::WriteAllText`
  to strip CR if you must round-trip.
- The parser is the acceptance bar: lexer changes must reproduce the game's tokenizer
  (see CODE_REVIEW #1 for the verified escape/comment semantics) and keep the test suite
  green. UI behavior is verified by the user manually; lint + build + tests are your gate.
