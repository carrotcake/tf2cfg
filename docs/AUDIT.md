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

10. ~~**No automated tests anywhere**~~ **FIXED** — `tests/tst_engine.cpp` + `tests/tests.pro` (Qt Test), 32 cases: cfgLex tokenization (quotes, escapes, comments, case folding, `;` inside quotes), cmdLex CSV mapping/round-trip/padding, the `allcmds.csv` load, `CommandListModel` bind replay (`unbindall`, `unbind`, later-wins, empty command, `setBind` replace/idempotence/case/escaping, `clearBind`), and `ConsoleCommandModel` sorting, the three filters, the per-combo cache, `suggest`, `findCommand` and `commandAt`. Built and run separately from the app, so a red test never blocks a release build:
   ```
   cd build-test && qmake ../tests/tests.pro && mingw32-make && release/tf2cfg_test.exe
   ```
   Still untested: the QML layer itself, which would mean QML Test and a new build target.

11. **`GameCfg::loadAllCmds()` mixes CSV parsing, type inference, and object creation** (`src/gamecfg/gameconfig.cpp:44-105`)
    - **What this is:** One 60-line function reads the CSV, decides the C++ subclass (`ConCmd` vs `BoolVar` vs `IntVar`...), parses min/max/def, and `new`s the object. Hard to test in isolation; any change to the type logic requires a full parse.
    - **Fix:** Split into: (a) CSV → flat `struct RawCmd { name, type, def, min, max, flags, docs, input, platform, context }`, (b) `RawCmd` → `ConVar*` factory. The factory is pure and unit-testable.
    - **If we skip it:** Hard to add a new input type (e.g. vector) without touching the parse loop.

12. **`Lexer` base class mixes file I/O, line storage, and cursor management** (`src/gamecfg/lex/lexers.h:10-50`)
    - **What this is:** `Lexer::open()` reads the whole file into `QStringList arr`; `cfgLex` and `cmdLex` inherit and add their own tokenizers. The base knows too much: it decides preprocessing (lowercase, comment strip) via a virtual `preprocess()` that only `cfgLex` overrides.
    - **Fix:** Make `Lexer` a pure cursor over a `QStringList` (or `QStringView` of the whole file). Move file reading to a free function. `cfgLex` gets its own reader that lowercases/strips comments; `cmdLex` reads raw. No virtual dispatch.
    - **If we skip it:** Current design works; adding a third lexer (e.g. for VPK) would repeat the same coupling.

13. ~~**Filter logic duplicated between `ConsoleCommandModel::rebuild()` and `suggest()`**~~ **NO LONGER TRUE** — the duplication was removed by the #4 fix (`d7dda22`): `suggest()` now walks the cached `visibleIndices` for the active filter combo and re-applies nothing. The three inline checks in `rebuild()` are the only copy left. Extract `passesFilters()` when a fourth filter actually arrives, not before.

14. ~~**Magic strings for bind-related commands scattered in multiple files**~~ **FIXED** — `BindSyntax` (`bind`, `bindtoggle`, `unbind`, `unbindall`) is now a set of `constexpr QLatin1StringView` constants in the anonymous namespace of `src/ui/commandlistmodel.cpp`, used by all five sites. Kept out of the header because no other translation unit needs them. One correction to the finding as written: the `"endcmd"` token in `configlexer.cpp` is the lexer's end-of-command marker, not a bind command, so it does not belong in that namespace. The `setBind()` writer and the `scanBinds()` reader now share one literal, which is the coupling that mattered.

15. ~~**`src/gamecfg/cvar.cpp` is a dead stub compiled into the binary**~~ **FIXED** — deleted, and dropped from `SOURCES`.

16. ~~**`CommandListModel::setBind()` lowercases the command, but TF2 aliases/commands can have case-sensitive arguments**~~ **FIXED** — only the first word is folded to lower case; the rest is stored as typed. Rows read from a file stay lower case, because `cfgLex` folds every line, so an edit applied twice still produces one identical row.

17. **Theme.qml not reviewed for hardcoded duplication** — **REVIEWED, NOTHING TO DO.** 76 lines, every literal a named token, each defined once; no colour or metric is duplicated anywhere in `qml/`.

18. **`BindEditorDialog.qml` is 765 lines — a single component doing too many jobs**
    - **What this is:** Filter dropdowns, editable command combo with suggestion pop-down, argument editor factory, toggle-vs-fixed radio group, custom command text area, OK/Cancel handling, drag header, centering logic. All in one file.
    - **Fix:** Extract `CommandFiltersBar.qml`, `CommandComboWithSuggestions.qml`, `ArgumentEditors.qml`, `CustomCommandTab.qml`. The dialog becomes a thin shell.
    - **If we skip it:** Hard to read, hard to test the argument editors in isolation.

### Nice to have (small speed-ups, shorter forms, lean)

19. ~~**One-off data-prep scripts in `tools/` can be deleted**~~ **FIXED** — `populate_desc.py`, `flags_verify.py`, `flags_update.py`, `add_fields.py`, `desc_part1..8.py` deleted. Two files stayed and the audit's "keep `csvio.py`" was not the whole reason: `field_dicts.py` holds the `SIGS`/`INPUT_OVERRIDES` tables that `res/cfg/schema.md` cites by name, so deleting it would dangle the dataset's own documentation. -12 files, ~1500 lines.

20. **`res/cfg/schema.md` and `flags.md` duplicate column meanings that could be a header row in the CSV**
    - **What this is:** The CSV is headerless by design (spec). The two `.md` files document the 10 columns and 32 flag tags. If the CSV ever gained a header row (commented or not), the docs could be generated from it.
    - **Fix:** Not worth changing the CSV format. Leave as is.

21. ~~**Widget UI in `src/legacy/widgets/` is dead code not built**~~ **FIXED** — deleted (`git rm -r`), history preserved. The parity contract in `docs/legacy/ui-qml-rewrite.md` section 3 cites these files as evidence, so that document now says how to pull one back out of git. -11 files, ~3000 lines.

22. **No Linux build verification in CI**
    - **What this is:** The `.pro` uses MinGW paths; Linux build assumes Qt 6 in standard paths. No GitHub Actions / GitLab CI to confirm it compiles on Ubuntu/Arch.
    - **Fix:** Add a minimal workflow: `ubuntu-latest`, install `qt6-base-dev`, `qmake`, `make`, run `qmllint`.
    - **If we skip it:** Linux users (half the TF2 player base) may hit compile errors on first try.

23. ~~**`windeployqt` command is not scripted**~~ **FIXED** — `deploy.ps1`: copies `build/<config>/tf2cfgreal.exe` into a clean `dist/`, runs `windeployqt --<config> --compiler-runtime --qmldir qml`, and fails loudly when the build or the Qt kit is missing. Verified: the deployed folder runs with no Qt directory on `PATH`. `-QtRoot`/`-Output`/`-Config` are parameters, not edits.

---

## Verdict

**Healthy core, safety net in place.** The engine and UI are functionally complete for the read-only use case. All nine "Must fix" items are resolved, `tests/tst_engine.cpp` guards the parser, the bind logic and the command catalog, and the dead code is gone. `docs/CODE_REVIEW.md` has no open findings left. The **biggest missing feature is save-to-disk**, which the docs already track.

**Next up, in order:**
1. CI (#22) — run `tf2cfg_test` and build the app on `ubuntu-latest` so half the player base is not the first to find a compile error.
2. Save-to-disk (docs/legacy/ui-qml-rewrite.md section 10) — the editor has been writable in memory since D16 with nowhere to write. Its prerequisite (argument case surviving a reload) is now in place; see the parse-fidelity note in `docs/CODE_REVIEW.md`.
3. `passesFilters()` (#13) — only when a fourth filter ships.
4. Not worth it unless something asks: #11 and #12 (refactors with no caller), #18 (cosmetic), #20 (leave as is).

## Lean

- **Done:** ~4500 lines of dead code removed — `src/legacy/widgets/` (3000), the one-off dataset scripts in `tools/` (1500), `src/gamecfg/cvar.cpp` (1).
- **0 dependencies removable:** Only Qt 6 (core, gui, quick, quickcontrols2, concurrent) — already minimal.
- **~500 lines to split** (still available, still not worth it): `BindEditorDialog.qml` → 4 subcomponents.

## Not checked

- `qml/keyboard/*.qml`, `qml/dialogs/BindEditorDialog.qml` — read where the work touched them, not end to end.
- `tools/csvio.py` — read; simple and correct.
- `docs/CODE_REVIEW.md` — **no open findings left.** Every finding is fixed or was retired with the widget UI, and its three "Info" items are answered against the behaviour as built.
- Linux build — not attempted; only Windows MinGW verified. **Note: added `concurrent` to QT in .pro for QtConcurrent.**
- Accessibility — see the QML read-through below; evaluated, and the gap is real.
- Packaging/installer — none exists; `deploy.ps1` (`windeployqt` folder copy) is the deploy, verified to run with no Qt on `PATH`.

## QML layer read-through (2026-10-12)

`qml/components/*.qml` (6 files) and `qml/pages/**` (5 files, 719 lines) read line by line for the first time. `qmllint` over the whole `qml/` tree reports no errors; its 41 remaining warnings are all the same `pragma ComponentBehavior: Bound` advisory, which predates this pass.

**Fixed**

1. `StartPage.localPath()` stripped the leading `/` from Unix paths (`file:///home/user/x.cfg` → `home/user/x.cfg`), so **every config picked through Browse failed on Linux** — a platform the app targets. The regex is now `^file:\/\/` plus a drive-letter test.
2. A failed load was never reported once a config was already loaded: `cfg.loaded` is `rowCount() > 0` and a failure leaves the old rows, so the success branch won. `lastError` is checked first now.
3. `CommandOverviewTab`'s delegates carried a `MouseArea` with a pointing-hand cursor and no click handler — parity P16 kept the cursor, but the area swallowed every press for nothing. Now a `HoverHandler`, which gives the cursor without taking input.
4. `Tf2ComboBox`'s indicator hit zone ignored `control.enabled`, so a disabled combo would still open its popup. Latent — no caller disables a combo today.
5. Six metrics moved into `Theme` (tab height and minimum width, combo default width, popup max height, status bar height) and the combo popup height stopped hardcoding `+ 2` for its own padding.
6. `CommandOverviewTab`'s scroll bar was the app's only unstyled scrollbar; it uses Theme colours now. The dialog's suggestion indicator and horizontal scroll bar are still default-styled.
7. `parseInt` in `BindEditorDialog.validBottom/validTop` had no radix, so `0x10` parsed as 16.

**Open, in order of value**

- **No `Accessible.*` anywhere in `qml/`** — zero of six components and five pages carry an accessible name or role. Every button, combo, field and the custom tab strip is invisible to assistive tech. No engine test will ever catch this.
- **`Tf2TabBar` has no keyboard path** — no `activeFocusOnTab`, no `Keys`, no `KeyNavigation`; it is `Item` + `MouseArea`, so the three editor tabs are mouse-only. Prev/Next buttons still work, so it is not a dead end. The dialog's command combo is the only `Keys` handler in the app.
- **`Main.qml:18` — the one real `qmllint` warning:** `pageStack.currentItem.pageTitle` is a member lookup on `QQuickItem`, which has no such property. It works at runtime because both pages declare it, but the type system does not check it.
- `StartPage.localPath()` calls `decodeURIComponent` unguarded, so a filename containing a bare `%` raises a `URIError` and breaks the JS handler. Rare.
- The 41 `ComponentBehavior: Bound` advisories. Cosmetic, repo-wide, and each needs a decision per file rather than a blanket change.

## Checked since this audit was written

- `qml/theme/Theme.qml` — 84 named tokens, nothing duplicated (#17).
- `res/res.qrc` and `qml.qrc` — complete: all 17 `.qml` files and both `qmldir`s are packed, as are all four presets, all four fonts and `allcmds.csv`. `flags.md`/`schema.md` are correctly absent; no code reads them.
- `res/cfg/allcmds.csv` — all 3825 rows carry exactly 10 fields under a real CSV parse.
- `qml/components/*.qml` and `qml/pages/**` — see the QML layer read-through below.