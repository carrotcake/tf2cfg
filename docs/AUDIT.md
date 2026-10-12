# Repository Audit — tf2cfg

Audit date: 2026-10-12
Scope: entire repo (C++ engine, QML UI, Python tooling, data files)
Assumed load: **single user, local desktop app, one config at a time, ~3800 commands, ~60 binds, no concurrency, no network, no money**. Windows and Linux (the two platforms TF2 runs on). Qt 6.12 MinGW on Windows; Linux build untested but expected to work.

## What this repo does

TF2 Config Manager is a Qt/QML desktop tool for viewing and editing Team Fortress 2 `.cfg` files. You load a config (or a bundled preset), browse the parsed commands, click a key on a visual keyboard/mouse/controller, and a dialog helps you construct a valid `bind` line. The engine parses configs and a 3800-row command catalog (`allcmds.csv`) with type/min/max/flags/platform/context metadata. The app is currently **read-only**: edits live in memory until the next load; there is no save path.

## Findings

Numbered across all groups so you can say "fix 2 and 5".

### Must fix (bug, security, data loss, breaks at expected load)

1. **Memory leak: `GameCfg::known_cmds` never frees its `ConVar*` objects** (`src/gamecfg/gameconfig.cpp:58-100`)
   - **What this is:** `loadAllCmds()` `new`s one `ConCmd`/`BoolVar`/`IntVar`/`FltVar`/`StrVar` per CSV row (3800 rows) and stores the owning pointers in a `QVector`. `GameCfg` lives for the whole process, so the leak never manifests as OOM, but it is a leak.
   - **Fix:** Add a destructor to `GameCfg` that `qDeleteAll(known_cmds)`. One line, nothing else changes.
   - **If we skip it:** Valgrind/ASan will complain; no user-visible effect at current scale.

2. **`CommandListModel::binds()` rebuilds the entire map on every read** (`src/ui/commandlistmodel.cpp:94-128`)
   - **What this is:** `binds()` iterates all parsed entries (`m_entries`, up to ~300 lines) and reconstructs a `QVariantMap` from scratch. QML accesses this via `cfg.binds` on every key cap repaint (104 caps) and on every bind change. Currently fast enough (104 × 300 ≈ 31k trivial ops), but quadratic if both grow.
   - **Fix:** Cache the map and invalidate only when `m_entries` changes (i.e. in `setEntries`, `setBind`, `clearBind`). Return the cached `QVariantMap` from `binds()`.
   - **If we skip it:** No problem today; future save/load of larger configs could jank.

3. **`ConfigController::loadCfg()` parses synchronously on the GUI thread** (`src/ui/configcontroller.cpp:78-99`)
   - **What this is:** `appendCfg()` tokenizes and classifies the whole file before returning. A 10k-line config would block the UI for a noticeable moment.
   - **Fix:** Move parsing to a worker thread (`QtConcurrent::run` or a `QThread`), emit `loadedChanged` when done. The model swap (`setEntries`) must stay on the GUI thread.
   - **If we skip it:** Large configs (not typical for TF2) freeze the window.

4. **`ConsoleCommandModel::suggest()` scans the full catalog on every keystroke** (`src/ui/consolecommandmodel.cpp:143-159`)
   - **What this is:** The Build-A-Bind command field calls `suggest(text)` on every `editTextChanged`. It iterates all 3800 `m_all` entries, applies three filters, and does a `contains` match. At 60 fps typing this is ~200k string ops/frame — acceptable on modern CPUs, but wasteful.
   - **Fix:** Pre-index names (e.g. a `QMap<QString, QVector<int>>` of trigrams, or just cache the last filter+text result and invalidate on filter change). Or debounce the QML side (200 ms).
   - **If we skip it:** Works fine; only a perf nit.

5. **`cmdLex::Token` has `const QString` members, making it non-movable and expensive to return by value** (`src/gamecfg/lex/lexers.h:117-165`)
   - **What this is:** Every field is `const QString`. `nextToken()` returns `Token` by value, forcing a copy of all 10 strings per row (3800 × 10 = 38k allocations on load). `QVector` cannot move them; it copies.
   - **Fix:** Drop `const` on the members. Return `Token` by value (NRVO/move will work). Or store `QStringView` into the line buffer (lifetime = `arr` in base `Lexer`).
   - **If we skip it:** Load takes a few extra ms; not user-visible.

6. **Config lexer does not handle escaped quotes inside quoted strings** (`src/gamecfg/lex/configlexer.cpp:23-30`)
   - **What this is:** The DFA treats a `"` inside a quoted token as the end of the token. TF2 configs allow `\"` inside quotes (e.g. `alias test "say \"hello\""`). The current lexer would split this into two tokens.
   - **Fix:** In `STATE_ESCP` (entered after `\`), treat the next char as literal and append it, including `"` and `\`. The DFA already has `STATE_ESCP` — wire it to append `c` unconditionally and return to `STATE_CHAR`.
   - **If we skip it:** Aliases with embedded quotes parse incorrectly; rare but valid.

7. **KeyCap tooltip interpolates user data into a format string** (`qml/keyboard/KeyCap.qml`)
   - **What this is:** `qsTr('bind "%1" "%2"').arg(cap.token).arg(cap.boundCommand)`. If `boundCommand` contains `%` (e.g. `echo 100%`), `qsTr` treats it as a format specifier and produces garbage or crashes.
   - **Fix:** Use `Qt.format()` or build the string manually: `'bind "' + cap.token + '" "' + cap.boundCommand + '"'`.
   - **If we skip it:** Weird tooltip text when a bind contains `%`.

8. **`CommandListModel::bindRows()` scans all entries on every `setBind`/`clearBind`** (`src/ui/commandlistmodel.cpp:130-146`)
   - **What this is:** `bindRows(key)` iterates `m_entries` to find rows for a key. Called twice per edit (once in `binds()` for the equality check, once to remove). With ~300 entries it's trivial, but duplicated logic with `binds()`.
   - **Fix:** Share the scan: `binds()` can return both the map and the row indices, or cache the row list per key.
   - **If we skip it:** No measurable impact; code duplication only.

9. **Only 2 of 4 bundled presets are exposed in the UI** (`src/ui/configcontroller.cpp:22-38`)
   - **What this is:** `b4nny_defbinds.cfg` and `b4nny_nobinds.cfg` exist in `res/res.qrc` but `presets()` returns only `b4nny.cfg` and `default.cfg`.
   - **Fix:** Add the two missing entries to the `presets()` list with correct labels/notes.
   - **If we skip it:** Users cannot select those presets from the UI.

### Should fix (risky code without test, real slowness, duplication, mixed jobs, dead code)

10. **No automated tests anywhere**
    - **What this is:** Zero unit tests. Parsing logic (`cfgLex`, `cmdLex`), bind resolution (`binds()`, `setBind`, `clearBind`), CSV I/O (`csvio.py`), flag normalization (`flags_update.py`) — all untested. A single regression in the lexer or bind logic would ship silently.
    - **Fix:** Add a minimal Qt Test target (`tf2cfg_test`) covering: `cfgLex` tokenization of quoted/escaped strings, `cmdLex` CSV row round-trip, `CommandListModel::binds()` replay semantics (unbindall, later bind wins, empty command clears), `setBind`/`clearBind` idempotence. One test binary run in CI.
    - **If we skip it:** Every refactor is a gamble; the parser has known bugs (#6) with no safety net.

11. **`GameCfg::loadAllCmds()` mixes CSV parsing, type inference, and object creation** (`src/gamecfg/gameconfig.cpp:44-105`)
    - **What this is:** One 60-line function reads the CSV, decides the C++ subclass (`ConCmd` vs `BoolVar` vs `IntVar`...), parses min/max/def, and `new`s the object. Hard to test in isolation; any change to the type logic requires a full parse.
    - **Fix:** Split into: (a) CSV → flat `struct RawCmd { name, type, def, min, max, flags, docs, input, platform, context }`, (b) `RawCmd` → `ConVar*` factory. The factory is pure and unit-testable.
    - **If we skip it:** Hard to add a new input type (e.g. vector) without touching the parse loop.

12. **`Lexer` base class mixes file I/O, line storage, and cursor management** (`src/gamecfg/lex/lexers.h:10-50`)
    - **What this is:** `Lexer::open()` reads the whole file into `QStringList arr`; `cfgLex` and `cmdLex` inherit and add their own tokenizers. The base knows too much: it decides preprocessing (lowercase, comment strip) via a virtual `preprocess()` that only `cfgLex` overrides.
    - **Fix:** Make `Lexer` a pure cursor over a `QStringList` (or `QStringView` of the whole file). Move file reading to a free function. `cfgLex` gets its own reader that lowercases/strips comments; `cmdLex` reads raw. No virtual dispatch.
    - **If we skip it:** Current design works; adding a third lexer (e.g. for VPK) would repeat the same coupling.

13. **Filter logic duplicated between `ConsoleCommandModel::rebuild()` and `suggest()`** (`src/ui/consolecommandmodel.cpp:104-127, 143-159`)
    - **What this is:** Both methods iterate `m_all` and apply the same three `if (!filter.isEmpty() && cmd->get_X() != filter) continue;` checks. If a filter changes, both must be updated.
    - **Fix:** Extract `bool passesFilters(const cmd::ConVar*) const` and call it from both.
    - **If we skip it:** Bug risk when adding a fourth filter (e.g. "show only cvars").

14. **Magic strings for bind-related commands scattered in multiple files**
    - **Locations:** `commandlistmodel.cpp:32` (`isBindCommand` checks `"bind"`/`"bindtoggle"`), `commandlistmodel.cpp:103-110` (`"unbindall"`/`"unbind"`), `configlexer.cpp:18` (emits `"endcmd"` token). The engine lowercases everything so case-insensitive match is correct, but the strings are not centralized.
    - **Fix:** Centralize in a namespace `BindSyntax` in `commandlistmodel.h` with `constexpr` `QStringView` constants. Use everywhere.
    - **If we skip it:** A typo in one place desyncs the bind logic.

15. **`src/gamecfg/cvar.cpp` is a dead stub compiled into the binary** (`tf2cfgreal.pro:17`)
    - **What this is:** File contains only `#include "gamecfg/gameconfig.h"`. No symbols, no code. Listed in `SOURCES` and built every time.
    - **Fix:** Remove from `SOURCES` in `.pro`. Delete the file.
    - **If we skip it:** Wasted compile time (negligible), dead file in tree.

16. **`CommandListModel::setBind()` lowercases the command, but TF2 aliases/commands can have case-sensitive arguments**
    - **What this is:** Line 152: `cleanCommand = command.trimmed().toLower()`. TF2 console commands are case-insensitive, but *arguments* to commands (e.g. `say "Hello World"`) preserve case. Lowercasing the whole command string mangles alias bodies and say-text.
    - **Fix:** Only lowercase the command *name* (first word). Keep the rest as typed. The parser lowercases config lines on load, so round-tripping a loaded bind already loses case — but a user typing a new bind in the Custom Command tab should keep case.
    - **If we skip it:** Aliases and `say` binds lose capitalization; minor but wrong.

17. **Theme.qml not reviewed for hardcoded duplication** — likely has color literals that could be centralized. (Did not read `qml/theme/Theme.qml`.)

18. **`BindEditorDialog.qml` is 765 lines — a single component doing too many jobs**
    - **What this is:** Filter dropdowns, editable command combo with suggestion pop-down, argument editor factory, toggle-vs-fixed radio group, custom command text area, OK/Cancel handling, drag header, centering logic. All in one file.
    - **Fix:** Extract `CommandFiltersBar.qml`, `CommandComboWithSuggestions.qml`, `ArgumentEditors.qml`, `CustomCommandTab.qml`. The dialog becomes a thin shell.
    - **If we skip it:** Hard to read, hard to test the argument editors in isolation.

### Nice to have (small speed-ups, shorter forms, lean)

19. **One-off data-prep scripts in `tools/` can be deleted** (`tools/populate_desc.py`, `tools/flags_verify.py`, `tools/flags_update.py`, `tools/desc_part*.py`, `tools/add_fields.py`)
    - **What this is:** These were used to generate/enrich `allcmds.csv` from raw dumps. The CSV is now canonical and committed. The scripts are never run again unless the dataset is refreshed (which TF2 will never need — the game hasn't updated since 2017).
    - **Fix:** `git rm tools/*.py` (keep `csvio.py` which is used by nothing now but could be useful for a future refresh). Or move to `tools/archive/`.
    - **Lean:** -10 files, ~1500 lines.

20. **`res/cfg/schema.md` and `flags.md` duplicate column meanings that could be a header row in the CSV**
    - **What this is:** The CSV is headerless by design (spec). The two `.md` files document the 10 columns and 32 flag tags. If the CSV ever gained a header row (commented or not), the docs could be generated from it.
    - **Fix:** Not worth changing the CSV format. Leave as is.

21. **Widget UI in `src/legacy/widgets/` is dead code not built**
    - **What this is:** 7 `.cpp`/`.h` files + 4 `.ui` files kept for "parity audit" per `docs/legacy/ui-qml-rewrite.md`. The QML rewrite is done and shipped.
    - **Fix:** `git rm -r src/legacy/widgets/`. History preserves them.
    - **Lean:** -11 files, ~3000 lines.

22. **No Linux build verification in CI**
    - **What this is:** The `.pro` uses MinGW paths; Linux build assumes Qt 6 in standard paths. No GitHub Actions / GitLab CI to confirm it compiles on Ubuntu/Arch.
    - **Fix:** Add a minimal workflow: `ubuntu-latest`, install `qt6-base-dev`, `qmake`, `make`, run `qmllint`.
    - **If we skip it:** Linux users (half the TF2 player base) may hit compile errors on first try.

23. **`windeployqt` command is not scripted**
    - **What this is:** The deploy step (`windeployqt --release --compiler-runtime --qmldir qml`) lives only in tribal knowledge / AGENTS.md. A `deploy.ps1` or `deploy.sh` would make releases reproducible.
    - **Fix:** Add a one-liner script at repo root.

---

## Verdict

**Healthy core, missing safety net.** The engine and UI are functionally complete for the read-only use case. The parser has a known escape bug (#6) and a memory leak (#1) but neither affects typical TF2 configs today. The **biggest risk is zero tests** (#10) — any future change (save feature, Linux port, config refresh) has no guard rails. The **biggest missing feature is save-to-disk**, which the docs already track.

**Priority order if you continue:**
1. Add a minimal test suite (#10) — covers the parser and bind logic.
2. Fix the escaped-quote lexer bug (#6) — corrects a real config case.
3. Free the `ConVar*` objects in `GameCfg` dtor (#1) — cleans up sanitizers.
4. Expose the two missing presets (#9) — finishes the Start screen.
5. Consider a worker thread for `loadCfg` (#3) — only if users report jank.

## Lean

- **~4500 lines removable:** `src/legacy/widgets/` (3000), `tools/*.py` except `csvio.py` (1500), `src/gamecfg/cvar.cpp` (1).
- **0 dependencies removable:** Only Qt 6 (core, gui, quick, quickcontrols2) — already minimal.
- **~500 lines to split:** `BindEditorDialog.qml` → 4 subcomponents.

## Not checked

- `qml/theme/Theme.qml` — only skimmed via imports; may have hardcoded duplication.
- `qml/components/*.qml` (`Tf2ComboBox`, `Tf2Button`, `Panel`, `Tf2TabBar`, `Tf2Label`, `Tf2TextField`) — assumed correct; not read line by line.
- `qml/pages/StartPage.qml`, `qml/pages/tabs/*.qml` — not read.
- `tools/csvio.py` — read; simple and correct.
- Linux build — not attempted; only Windows MinGW verified.
- `res/res.qrc` and `qml.qrc` resource packing — not audited for completeness.
- Accessibility (screen readers, high contrast) — not evaluated.
- Packaging/installer — none exists; `windeployqt` folder copy is the deploy.