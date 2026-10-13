# Repository Audit — tf2cfg

Audit date: 2026-10-12 (updated in place as the work landed; every claim below is checked
against the tree it describes).
Scope: entire repo (C++ engine, QML UI, data files, and the two surviving files in `tools/`).
Assumed load: **single user, local desktop app, one config at a time, 3825 commands, 54-67 binds, no concurrency, no network, no money**. Windows and Linux (the two platforms TF2 runs on). Qt 6.12 MinGW on Windows; Linux build untested but expected to work.

## What this repo does

TF2 Config Manager is a Qt/QML desktop tool for viewing and editing Team Fortress 2 `.cfg` files. You load a config (or a bundled preset), browse the parsed commands, click a key on a visual keyboard/mouse/controller, and a dialog helps you construct a valid `bind` line. The engine parses configs and a 3800-row command catalog (`allcmds.csv`) with type/min/max/flags/platform/context metadata. The app is currently **read-only**: edits live in memory until the next load; there is no save path.

## Findings

Numbered across all groups so you can say "fix 2 and 5".

Every item carries one of three states: **fixed**, **closed with no change** (decided against, with
the reason recorded — these are not a to-do list and should not be re-raised), or **open**.

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
   - **Verified:** all four bundled configs tokenize identically before and after the change (diffed old vs new lexer build over every token); the escaped-quote case is pinned by `tests/tst_engine.cpp`. The later case-fold change (CODE_REVIEW #1) was verified the same way — also identical, because no bundled config has a mixed-case argument.

7. ~~**KeyCap tooltip interpolates user data into a format string**~~ **FIXED** — changed to string concatenation: `'bind "' + cap.token + '" "' + cap.boundCommand + '"'`.

8. ~~**`CommandListModel::bindRows()` scans all entries on every `setBind`/`clearBind`**~~ **FIXED** — added `scanBinds()` shared helper used by both `binds()` and `bindRows()`.

9. ~~**Only 2 of 4 bundled presets are exposed in the UI**~~ **FIXED** — added `b4nny_defbinds.cfg` and `b4nny_nobinds.cfg` to `presets()`.

### Should fix (risky code without test, real slowness, duplication, mixed jobs, dead code)

10. ~~**No automated tests anywhere**~~ **FIXED** — `tests/tst_engine.cpp` + `tests/tests.pro` (Qt Test), 32 cases: cfgLex tokenization (quotes, escapes, comments, case folding, `;` inside quotes), cmdLex CSV mapping/round-trip/padding, the `allcmds.csv` load, `CommandListModel` bind replay (`unbindall`, `unbind`, later-wins, empty command, `setBind` replace/idempotence/case/escaping, `clearBind`), and `ConsoleCommandModel` sorting, the three filters, the per-combo cache, `suggest`, `findCommand` and `commandAt`. Built and run separately from the app, so a red test never blocks a release build:
   ```
   cd build-test && qmake ../tests/tests.pro && mingw32-make && release/tf2cfg_test.exe
   ```
   Still untested: the QML layer itself, which would mean QML Test and a new build target.

11. ~~**`GameCfg::loadAllCmds()` mixes CSV parsing, type inference, and object creation**~~ **CLOSED — no change.** Splitting it into a `RawCmd` struct and a pure factory would move ~60 lines of working code and make it unit-testable — but it is already covered end to end by `catalogLoadsEveryRowAsACommand`, which loads all 3825 rows through the real function and checks one of each type. There is no third input type coming; a vector cvar would be a data change, not a structural one. Revisit only if the type switch grows a second decision axis.

12. ~~**`Lexer` base class mixes file I/O, line storage, and cursor management**~~ **CLOSED — no change.** The fix would delete the `virtual preprocess()` that `cfgLex` needs for the quote-aware comment strip and the command-name case fold (audit #6, CODE_REVIEW #1). The base reading a file and a subclass overriding one hook is the coupling that makes those two behaviours testable at all; splitting it would move the knowledge rather than remove it. Revisit only if a third lexer appears.

13. ~~**Filter logic duplicated between `ConsoleCommandModel::rebuild()` and `suggest()`**~~ **CLOSED — no change.** The duplication this described was removed by the #4 fix (`d7dda22`): `suggest()` walks the cached `visibleIndices` and re-applies nothing. The three inline checks in `rebuild()` are now the only copy, and `catalogSortsByAlphanumericName` through `findCommandIgnoresCaseAndFilters` pin their behaviour. Extract `passesFilters()` when a fourth filter ships.

14. ~~**Magic strings for bind-related commands scattered in multiple files**~~ **FIXED** — `BindSyntax` (`bind`, `bindtoggle`, `unbind`, `unbindall`) is now a set of `constexpr QLatin1StringView` constants in the anonymous namespace of `src/ui/commandlistmodel.cpp`, used by all five sites. Kept out of the header because no other translation unit needs them. One correction to the finding as written: the `"endcmd"` token in `configlexer.cpp` is the lexer's end-of-command marker, not a bind command, so it does not belong in that namespace. The `setBind()` writer and the `scanBinds()` reader now share one literal, which is the coupling that mattered.

15. ~~**`src/gamecfg/cvar.cpp` is a dead stub compiled into the binary**~~ **FIXED** — deleted, and dropped from `SOURCES`.

16. ~~**`CommandListModel::setBind()` lowercases the command, but TF2 aliases/commands can have case-sensitive arguments**~~ **FIXED** — only the first word is folded to lower case; the rest is stored as typed. Rows read from a file stay lower case, because `cfgLex` folds every line, so an edit applied twice still produces one identical row.

17. ~~**Theme.qml not reviewed for hardcoded duplication**~~ **CLOSED — no change.** Reviewed: 54 named tokens (25 colours), each defined once, and no colour or metric is duplicated anywhere in `qml/`. The one thing the review turned up was six *metrics* outside the theme, which are now tokens too (see the QML read-through below).

18. ~~**`BindEditorDialog.qml` is 811 lines — a single component doing too many jobs**~~ **CLOSED — no change.** Splitting it means four components each relaying ~10 properties and functions (`commandInfo`, `suggestionList`, `suggestionIndex`, `regen`, `innerCommand`, `settleCommand`), on the one screen with real interaction logic and no test to catch a broken binding. The functions are already separated by comment blocks and the file is read top to bottom; the line count is the only complaint. Revisit if the dialog grows a third tab.

### Nice to have (small speed-ups, shorter forms, lean)

19. ~~**One-off data-prep scripts in `tools/` can be deleted**~~ **FIXED** — `populate_desc.py`, `flags_verify.py`, `flags_update.py`, `add_fields.py`, `desc_part1..8.py` deleted. Two files stayed and the audit's "keep `csvio.py`" was not the whole reason: `field_dicts.py` holds the `SIGS`/`INPUT_OVERRIDES` tables that `res/cfg/schema.md` cites by name, so deleting it would dangle the dataset's own documentation. -12 files, ~1500 lines.

20. ~~**`res/cfg/schema.md` and `flags.md` duplicate column meanings that could be a header row in the CSV**~~ **CLOSED — no change.** The CSV is headerless by design (schema spec) and the dataset is frozen: the game has not updated since 2017. A header row would be a format change to a file 3825 rows and several tooling scripts depend on, to serve documentation that already exists and is accurate.

21. ~~**Widget UI in `src/legacy/widgets/` is dead code not built**~~ **FIXED** — deleted (`git rm -r`, 13 files), history preserved. The parity contract in `docs/legacy/ui-qml-rewrite.md` section 3 cites those files as evidence, so that document now says how to pull one back out of git. ~3000 lines.

22. **No Linux build verification in CI** — the only open finding here.
    - **What this is:** The `.pro` uses MinGW paths; Linux build assumes Qt 6 in standard paths. No GitHub Actions / GitLab CI to confirm it compiles on Ubuntu/Arch.
    - **Fix:** Add a minimal workflow: `ubuntu-latest`, install `qt6-base-dev`, `qmake`, `make`, run `qmllint`.
    - **Note added since:** `qmllint` is clean of errors today but reports 23 `ComponentBehavior` advisories, so a gate has to tolerate warnings rather than fail on them. The test target should be fine on Linux — it links only Qt core + testlib and the same resource bundle — but nothing has run there yet, which is the point of this finding.
    - **If we skip it:** Linux users (half the TF2 player base) may hit compile errors on first try.

23. ~~**`windeployqt` command is not scripted**~~ **FIXED** — `deploy.ps1`: copies `build/<config>/tf2cfgreal.exe` into a clean `dist/`, runs `windeployqt --<config> --compiler-runtime --qmldir qml`, and fails loudly when the build or the Qt kit is missing. Verified: the deployed folder runs with no Qt directory on `PATH`. `-QtRoot`/`-Output`/`-Config` are parameters, not edits.

---

## Verdict

**Healthy core, safety net in place.** The engine and UI are functionally complete for the read-only use case. All nine "Must fix" items are resolved, `tests/tst_engine.cpp` guards the parser, the bind logic and the command catalog, and the dead code is gone. `docs/CODE_REVIEW.md` has no open findings left. The **biggest missing feature is save-to-disk**, which the docs already track.

**Next up, in order:**
1. CI (#22) — run `tf2cfg_test` and build the app on `ubuntu-latest` so half the player base is not the first to find a compile error. The **only** open finding in this document.
2. Save-to-disk (docs/legacy/ui-qml-rewrite.md section 10) — the editor has been writable in memory since D16 with nowhere to write. Its prerequisite (argument case surviving a reload) is in place; see the parse-fidelity note in `docs/CODE_REVIEW.md`. Three decisions come first: full rewrite versus a surgical edit of the bind lines, overwrite versus confirm, and what to do if the file changed on disk while it was loaded.
3. Accessibility (`docs/accessibility.md`) — evaluated and sized, put off deliberately. The work items are ordered there; the two that matter are a focus ring on `Tf2Button` and making the 104 key caps reachable by keyboard.

Closed by decision and not to be re-raised: #11, #12, #13, #17, #18, #20 — each with its reason inline.

## Lean

- **Done:** ~4500 lines of dead code removed — `src/legacy/widgets/` (3000), the one-off dataset scripts in `tools/` (1500), `src/gamecfg/cvar.cpp` (1).
- **0 dependencies removable:** Only Qt 6 (core, gui, quick, quickcontrols2, concurrent) — already minimal.
- **Nothing left to split:** `BindEditorDialog.qml` is the one large file, and #18 closed the case for splitting it.

## Not checked

- `qml/keyboard/KeyboardPanel.qml` — read in full. `KeyCap.qml` and `qml/dialogs/BindEditorDialog.qml` — read where the work touched them, not end to end.
- `tools/csvio.py` — read; simple and correct.
- `docs/CODE_REVIEW.md` — **no open findings left.** Every finding is fixed or was retired with the widget UI, and its three "Info" items are answered against the behaviour as built.
- Linux build — not attempted; only Windows MinGW verified. **Note: added `concurrent` to QT in .pro for QtConcurrent.**
- Accessibility — **evaluated in `docs/accessibility.md`**, which also sizes the work. Put off deliberately: the gap is real (the key caps and tabs are keyboard-inaccessible) and nothing in the engine blocks it, but no one has asked for it.
- Packaging/installer — none exists; `deploy.ps1` (`windeployqt` folder copy) is the deploy, verified to run with no Qt on `PATH`.

## QML layer read-through (2026-10-12)

`qml/components/*.qml` (6 files) and `qml/pages/**` (5 files, ~750 lines) read line by line for the first time, followed by a leftovers pass over the rest of the tree. `qmllint` reports **0 errors**; its 23 remaining warnings are all `pragma ComponentBehavior: Bound` advisories, down from 41 before the pragma was added to the seven files that needed it.

**Fixed**

1. `StartPage.localPath()` stripped the leading `/` from Unix paths (`file:///home/user/x.cfg` → `home/user/x.cfg`), so **every config picked through Browse failed on Linux** — a platform the app targets. The regex is now `^file:\/\/` plus a drive-letter test.
2. A failed load was never reported once a config was already loaded: `cfg.loaded` is `rowCount() > 0` and a failure leaves the old rows, so the success branch won. `lastError` is checked first now.
3. `CommandOverviewTab`'s delegates carried a `MouseArea` with a pointing-hand cursor and no click handler — parity P16 kept the cursor, but the area swallowed every press for nothing. Now a `HoverHandler`, which gives the cursor without taking input.
4. `Tf2ComboBox`'s indicator hit zone ignored `control.enabled`, so a disabled combo would still open its popup. Latent — no caller disables a combo today.
5. Five metrics moved into `Theme` (tab height and minimum width, combo default width, popup max height, status bar height) and the combo popup height stopped hardcoding `+ 2` for its own padding.
6. All three scroll affordances are themed now: the command list's bar, the dialog's suggestion indicator and the dialog's argument bar. `CommandOverviewTab`'s was the last unstyled one.
7. `parseInt` in `BindEditorDialog.validBottom/validTop` had no radix, so `0x10` parsed as 16.
8. `Main.qml` read `pageStack.currentItem.pageTitle`, a member lookup on `QQuickItem` that has no such property — the one real `qmllint` warning in the tree. Titles live in `Main.qml` now, and the unused `pageTitle` on both pages is gone.
9. `localPath()` called `decodeURIComponent` unguarded, so a filename holding a bare `%` raised a `URIError` and broke the handler. It falls back to the undecoded string, which is still a usable path.
10. `pragma ComponentBehavior: Bound` on seven files: 41 advisories to 23, no errors.

**Open, in order of value**

- **Accessibility** — evaluated and sized in `docs/accessibility.md`. Put off deliberately.
- `Tf2TabBar` has no keyboard path — no `activeFocusOnTab`, no `Keys`, no `KeyNavigation`; it is `Item` + `MouseArea`, so the three editor tabs are mouse-only. Prev/Next buttons still work, so it is not a dead end. Folded into `docs/accessibility.md` (finding A2) rather than tracked twice.
- The remaining 18 `ComponentBehavior` advisories: context-property reads (`cfg.loaded`, `cfg.presets`) and `KeyboardPanel` delegates reaching an enclosing component's IDs. Both need a structural change — injecting the controller as a QML singleton, or threading properties through the panel's delegates — not a pragma.

## Checked since this audit was written

- `qml/theme/Theme.qml` — 54 named tokens (25 colours), nothing duplicated (#17).
- `res/res.qrc` and `qml.qrc` — complete: all 17 `.qml` files and both `qmldir`s are packed, as are all four presets, all four fonts and `allcmds.csv`. `flags.md`/`schema.md` are correctly absent; no code reads them.
- `res/cfg/allcmds.csv` — all 3825 rows carry exactly 10 fields under a real CSV parse.
- `qml/components/*.qml` and `qml/pages/**` — see the QML layer read-through above.
- Window sizing — not an audit finding, changed on request: one shared 1000×500 size for both screens, start screen fixed, editor resizable (deviation D2).