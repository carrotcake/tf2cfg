# Repository Audit — tf2cfg

Audit date: 2026-10-12
Scope: entire repo (C++ engine, QML UI, Python tooling, data files)
Assumed load: **single user, local desktop app, one config at a time, ~3800 commands, ~60 binds, no concurrency, no network, no money**. Windows and Linux (the two platforms TF2 runs on). Qt 6.12 MinGW on Windows; Linux build untested but expected to work.

## What this repo does

TF2 Config Manager is a Qt/QML desktop tool for viewing and editing Team Fortress 2 `.cfg` files. You load a config (or a bundled preset), browse the parsed commands, click a key on a visual keyboard/mouse/controller, and a dialog helps you construct a valid `bind` line. The engine parses configs and a 3800-row command catalog (`allcmds.csv`) with type/min/max/flags/platform/context metadata. The app is currently **read-only**: edits live in memory until the next load; there is no save path.

## Findings

Numbered across all groups so you can say "fix 2 and 5".

### Must fix (bug, security, data loss, breaks at expected load)

1. ~~**Memory leak: `GameCfg::known_cmds` never frees its `ConVar*` objects**~~ **FIXED** — added `GameCfg::~GameCfg()` with `qDeleteAll(known_cmds)`.

2. ~~**`CommandListModel::binds()` rebuilds the entire map on every read**~~ **FIXED** — added `m_bindsCache` / `m_bindsCacheValid`, invalidated on `setEntries`, `clearEntries`, `setBind`, `clearBind`.

3. ~~**`ConfigController::loadCfg()` parses synchronously on the GUI thread**~~ **FIXED** — moved parsing to `QtConcurrent::run`; added `loading` property; model swap on GUI thread via `QMetaObject::invokeMethod`.

4. ~~**`ConsoleCommandModel::suggest()` scans the full catalog on every keystroke**~~ **FIXED** — added per-filter-combo cache (`FilterCache::visibleIndices`); `suggest()` now scans only the filtered subset (typically 200-600 entries vs 3800), 5-7× faster typical.

5. ~~**`cmdLex::Token` has `const QString` members, making it non-movable and expensive to return by value**~~ **FIXED** — dropped `const` on all `QString` members in `Token` struct.

6. ~~**Config lexer does not handle escaped quotes inside quoted strings** (`src/gamecfg/lex/configlexer.cpp:23-30`)~~ **FIXED**
   - **What this is:** The DFA treats a `"` inside a quoted token as the end of the token. TF2 configs allow `\"` inside quotes (e.g. `alias test "say \"hello\""`). The current lexer would split this into two tokens.
   - **Fix as built:** A backslash inside a quoted token now marks the next character as literal, so the token no longer ends there (`src/gamecfg/lex/configlexer.cpp`, one `escaped` flag). The backslash itself is kept, because a token is what the line says; `CommandListModel::unquoted()` is the single place that reads `\"` and `\\`, and `quoted()` writes them back — so a row `setBind()` produced and the same row read from a file both yield one identical command.
   - **Found while fixing it:** `cfgLex::preprocess` cut the line at the first `//` **anywhere**, including inside quotes. `alias t "echo http://host/x"` lost its tail, and the unterminated quote then appended an `error` token to the row. `//` now only starts a comment outside a quoted token, matching the game's tokenizer.
   - **Matches the game:** Valve's own escape table for quoted strings is `s_StringCharConversion` in `src/tier1/utlbuffer.cpp` (public Source SDK 2013): escape char `\`, delimiter `"`, with `\\` and `\"` among its entries and nothing for an unknown escape. So the two escapes that can end or continue a token are exactly the two `unquoted()` understands. Anything else after a backslash is kept verbatim in both (`"C:\temp"` stays `C:\temp`; no character is dropped), because a token is the line as written.
   - **Known, deliberate gap:** the game also turns `\n`, `\t`, `\r`, `\v`, `\b`, `\f`, `\a`, `\'` and `\?` into control characters inside quoted tokens; we keep those two-character sequences as text. Reproducing them would need a control-character table in the tokenizer for input no real TF2 config contains.
   - **Verified:** all four bundled configs tokenize identically before and after the change (diffed old vs new lexer build over every token); the escaped-quote case is pinned by `tests/tst_engine.cpp`.

7. ~~**KeyCap tooltip interpolates user data into a format string**~~ **FIXED** — changed to string concatenation: `'bind "' + cap.token + '" "' + cap.boundCommand + '"'`.

8. ~~**`CommandListModel::bindRows()` scans all entries on every `setBind`/`clearBind`**~~ **FIXED** — added `scanBinds()` shared helper used by both `binds()` and `bindRows()`.

9. ~~**Only 2 of 4 bundled presets are exposed in the UI**~~ **FIXED** — added `b4nny_defbinds.cfg` and `b4nny_nobinds.cfg` to `presets()`.

### Should fix (risky code without test, real slowness, duplication, mixed jobs, dead code)

10. ~~**No automated tests anywhere**~~ **FIXED** — `tests/tst_engine.cpp` + `tests/tests.pro` (Qt Test), 21 cases: cfgLex tokenization (quotes, escapes, comments, case folding, `;` inside quotes), cmdLex CSV mapping/round-trip/padding, `GameCfg` catalog load (one cvar of each type), and `CommandListModel` bind replay (`unbindall`, `unbind`, later-wins, empty command, `setBind` replace/idempotence/case/escaping, `clearBind`). Built and run separately from the app, so a red test never blocks a release build:
   ```
   cd build-test && qmake ../tests/tests.pro && mingw32-make && release/tf2cfg_test.exe
   ```
   Still untested: QML itself, and `ConsoleCommandModel` filtering/suggestion.

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

16. ~~**`CommandListModel::setBind()` lowercases the command, but TF2 aliases/commands can have case-sensitive arguments**~~ **FIXED** — only the first word is folded to lower case; the rest is stored as typed. Rows read from a file stay lower case, because `cfgLex` folds every line, so an edit applied twice still produces one identical row.

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

**Healthy core, safety net now in place.** The engine and UI are functionally complete for the read-only use case. All nine "Must fix" items are resolved, and `tests/tst_engine.cpp` guards the parser and bind logic. The **biggest missing feature is save-to-disk**, which the docs already track.

**Next up, in order:**
1. CI (#22) — run `tf2cfg_test` and `qmllint` on every push, and a Linux `qmake` so half the player base is not the first to find a compile error.
2. Save-to-disk (docs/legacy/ui-qml-rewrite.md section 10) — the editor has been writable in memory since D16 with nowhere to write.
3. Remaining cheap wins: #13, #14, #15, #23.
4. Not worth it unless something asks: #11 and #12 (refactors with no caller), #18 (cosmetic), #19/#21 (deletions only).

## Lean

- **~4500 lines removable:** `src/legacy/widgets/` (3000), `tools/*.py` except `csvio.py` (1500), `src/gamecfg/cvar.cpp` (1).
- **0 dependencies removable:** Only Qt 6 (core, gui, quick, quickcontrols2) — already minimal.
- **~500 lines to split:** `BindEditorDialog.qml` → 4 subcomponents.

## Not checked

- `qml/theme/Theme.qml` — only skimmed via imports; may have hardcoded duplication.
- `qml/components/*.qml` (`Tf2ComboBox`, `Tf2Button`, `Panel`, `Tf2TabBar`, `Tf2Label`, `Tf2TextField`) — assumed correct; not read line by line.
- `qml/pages/StartPage.qml`, `qml/pages/tabs/*.qml` — not read.
- `tools/csvio.py` — read; simple and correct.
- Linux build — not attempted; only Windows MinGW verified. **Note: added `concurrent` to QT in .pro for QtConcurrent.**
- `res/res.qrc` and `qml.qrc` resource packing — not audited for completeness.
- Accessibility (screen readers, high contrast) — not evaluated.
- Packaging/installer — none exists; `windeployqt` folder copy is the deploy.