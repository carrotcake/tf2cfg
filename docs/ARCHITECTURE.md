# Architecture & Contracts - tf2cfg

The live contracts: what the engine must do, what the UI may assume, and the design
decisions that are still in force. Historical records (how we got here, verification
logs, retired findings) live in `docs/legacy/` and are not edited.

Layout, build/test commands and workflow rules are in `AGENTS.md`; this document holds
the contracts themselves.

## 1. Layer map

- `src/gamecfg/` - engine: config parser (`lex/`), `GameCfg`, `convars.h` (`cmd::Entry`,
  `cmd::ConVar` hierarchy). Deliberately UI-free.
- `src/ui/` - Qt objects: `ConfigController` (load/save state machine, context property
  `cfg`), `CommandListModel` (parsed rows), `ConsoleCommandModel` (catalog + suggest).
- `qml/` - the whole UI, embedded in `qml.qrc`. Theme tokens in `qml/theme/Theme.qml`.
- `res/res.qrc` - the command catalog `res/cfg/allcmds.csv` (3825 rows, 10 columns,
  headerless by design - schema in `res/cfg/schema.md`) and four bundled cfg presets.

Assumed load: single user, local, one config at a time, no network.

## 2. Parser contract

The parser is the acceptance bar: **lexer changes must reproduce the game's tokenizer.**
The verified semantics (`docs/legacy/CODE_REVIEW.md` #1, `docs/legacy/AUDIT.md` #6;
matches Valve's `s_StringCharConversion` in the Source SDK):

- The command name - the **first unquoted word** of the line - is case-folded; nothing
  else is. Argument case survives a load (the save path depends on it), and commands
  after a `;` on the same line keep their case (the lookup lowercases anyway).
- `//` starts a comment **only outside a quoted token** (URLs in quoted args survive).
- Inside a quoted token, `\"` and `\\` do not end it. Those are the only two escapes
  read or written; they live solely in `CommandListModel::unquoted()` / `quoted()`, so a
  row's command, the dialog's text and the same row re-read from a file are one identical
  string. Any other character after a backslash is kept verbatim (`"C:\temp"` stays
  `C:\temp`).
- The game also maps `\n`, `\t`, `\r`, `\v`, `\b`, `\f`, `\a`, `\'`, `\?` inside quoted
  tokens; we keep those two-character sequences as text - a deliberate gap, no real TF2
  config contains them.
- Lines are stored without separators and tokenization synthesizes a trailing `'\n'`; a
  quoted token therefore cannot span two lines (an unterminated quote is `TOK_ERR`,
  matching the game's line-oriented grammar). This is why `advanceOneChar()` bumps
  `line` as a side effect of end-of-line.
- Quoted tokens **keep their quote characters** in the parse - a token is the line as
  written; the Overview shows it verbatim and `unquoted()` strips for comparison.
- `cmdLex` (the CSV reader) reads raw - no case fold, no comment chop - and pads short
  rows to 10 empty fields instead of indexing blindly. Catalog descriptions are untouched.

## 3. Engine contracts

- **`GameCfg::appendCfg` accumulates.** Always `resetParsed()` before parsing another
  file into the same instance; `loadCfg` does this.
- `GameCfg()` (and the filename constructor, which delegates to it) loads
  `://cfg/allcmds.csv` before any row is parsed - `cmdToType()` is meaningless before
  that.
- `appendCfg()` returns `false` when the file cannot be read; `loadAllCmds()` warns when
  the resource bundle has no catalog. Failures must reach `ConfigController::lastError`,
  never show as a silently empty list.
- Type selection comes from the CSV input-type column (`bool`/`int`/`flt`/`str` →
  `BoolVar`/`IntVar`/`FltVar`/`StrVar`), not a guessing ladder. Empty-default string
  cvars are kept. `ConCmd::usage` carries the `[mandatory]/<optional>` signature.

## 4. Save contract (surgical save)

Full design and rationale: `docs/legacy/ui-qml-rewrite.md` section 10.1. The parse is
provably lossy (comments stripped, blank lines dropped, names case-folded), so a
wholesale model write would destroy user content. Therefore:

- **The file on disk is the source of truth for lines the app does not own; the model is
  the authority only for rows it owns.**
- `cmd::Entry` carries `int sourceLine` (file line it was parsed from; `-1` = created
  this session) and `bool appOwned` (set when this session created or edited the row).
  Ownership is a flag, not a command-name test.
- `ConfigController::writeConfig(path, source, error)`:
  1. Target is `cfgPath`; a `:/` preset path is refused (Save) / redirected (Save As).
  2. Source = target for Save, the loaded config for Save As; missing/empty source →
     render everything.
  3. mtime + size recorded at load are compared; a mismatch triggers the QML confirm
     dialog with both timestamps (sync-tool overwrite guard).
  4. `<name>.bak` is written beside the target before every write, through the
     two-temp-file swap (`docs/DESIGN-save-backup.md`): the copy lands at
     `<name>.bak.new`, then `.bak` -> `.bak.old` -> `.bak` renames swap it in - a
     failed copy or swap never destroys the previous restore point.
  5. Source bytes are split preserving line terminators (no `QIODevice::Text`) - CRLF
     stays CRLF, a missing final newline survives.
  6. Walk the lines: app-owned rows re-emit as `args.join(" ")` (what the Overview shows
     is what lands on disk); owned-but-deleted rows are dropped; everything else copies
     verbatim.
  7. Session rows (`sourceLine == -1`) insert after the last bind line (or at end of
     file when the config has no binds), so new binds stay with the binds.
  8. Write through `QSaveFile`; on failure set `lastError` and leave the `.bak`.
  9. On success re-record mtime/size.
- **Post-save resync** (`ConfigController::resyncAfterSave`, synchronous on purpose):
  every successful save re-parses the saved file and rebuilds rows + the `m_parsed`
  snapshot. Without it the next surgical save keyed rows to the old layout and could
  re-emit an edited session bind beside its old line. After the resync the saved path is
  the source for all later editing, every row carries its true file line, and `dirty`
  clears until the next edit.
- Accepted consequence: a comment sitting *between* two bind lines keeps its own file
  position while the surrounding binds are re-rendered in place.

## 5. Model contracts

### `CommandListModel` (parsed config rows)

- Row order = insertion order = parse order.
- Roles: `LineRole` (`args.join(" ")`), `CommandRole`, `ArgsRole`, `TypeRole`,
  `TypeNameRole`, `BoundKeyRole` (lowercase key token for `bind`/`bindtoggle` rows,
  else empty). Head-token test `isBindCommand` classifies bind rows (parsed binds are
  typed `CMD_CMD`).
- **`setBind(key, command)`** rebinds the key's most recent `bind` row in place (keeps
  `sourceLine`, sets `appOwned`), or drops all rows for the key and appends one
  `bind "<key>" "<command>"` row - so a key never ends up with two competing rows and a
  reload is indistinguishable from the edit. It stores the **bare command**, not a
  composed bind line, and lowercases only the first word (console names are
  case-insensitive, arguments are not - D18). Emits `bindsChanged`. As of the 2026-10-13
  audit fix (#1), the in-place path also drops every later `unbind`/`bindtoggle` row for
  the key, matching the append path. Overlapping `loadCfg` requests are ignored while a
  load is in flight (`docs/AUDIT.md` #2). `setBindToggle(key, command)` writes the same
  row with a `bindtoggle` head, and `bindingKind(key)`/`bindingKinds()` report whether
  the effective binding is `bind` or `bindtoggle` (`docs/AUDIT.md` #6).
- `binds()` replays the rows the way TF2 would: later bind wins; `unbindall`,
  `unbind "k"`, and an empty command clear. Returns lowercase key token → command.
- Saving reads the model via `CommandListModel::entries()`, **not** `m_cfg.getCmdList()`.
- `loadCfg` parses on a worker thread (`QtConcurrent::run`); the model swap happens on
  the GUI thread via `QMetaObject::invokeMethod`. Failed loads must not blank the old
  rows, and `lastError` is checked before `loaded` (`loaded` = `rowCount() > 0`, so a
  failed reload would otherwise win the success branch). A load already in flight makes
  subsequent `loadCfg` calls no-ops rather than racing on the shared `GameCfg`.

### `ConsoleCommandModel` (the catalog for Build-A-Bind)

- This is the *game's* command catalog; `CommandListModel` is the *parsed config* list.
  The two are intentionally separate and must not merge.
- Filters (`contextFilter`, `platformFilter`, `showCheats`) build a per-combo
  `FilterCache` of sorted visible indices; `suggest()` walks only that subset. `rebuild()`
  and `suggest()` must keep using identical filter logic - extract a shared helper if a
  fourth filter ships. Rows sort **context-first, then alphanumeric name** (the command
  picker groups by the `context` role through `Tf2ComboBox`'s opt-in `sectionRole` -
  headers need contiguous groups; pinned by `catalogSortsByContextThenName`).
- Per-argument bounds (`docs/DESIGN-arg-bounds.md`): `GameCfg` attaches `cmd::ArgSpec`s
  from `res/cfg/argbounds.csv` after the catalog loads (companion file, hand-verified
  rows only); the model exposes them through the `argsSpec` role and inside
  `commandAt`/`findCommand` maps. Cvar ranges keep flowing through `min`/`max`, now
  one-sided (a lone min or max survives). Bounds are authoring metadata only - the
  engine never clamps command arguments.

### `ConfigController` (context property `cfg`)

The historical parity table is `docs/legacy/ui-qml-rewrite.md` section 6; where that
table differs from this one, this one is current (`loadCfg` in particular is now async,
and `consoleModel`/`loading`/`presetNote` exist).

| Member | Kind | Contract |
|---|---|---|
| `presets` | `QVariantList` | `[{label, path, note}]`, four bundled presets |
| `presetAt(index)` / `presetNote(index)` | `Q_INVOKABLE QString` | `:/cfg/...` path / note, or empty |
| `loadCfg(path)` | `Q_INVOKABLE void` | **async**: parses on a `QtConcurrent` worker; `loading` goes true then false, `loadedChanged`/`lastErrorChanged` fire when done. Failure keeps the old rows and sets `lastError` |
| `clear()` | `Q_INVOKABLE void` | empties the model and load state |
| `commandModel` / `consoleModel` | `Q_PROPERTY QAbstractItemModel*` | parsed-config rows / the allcmds catalog (`CONSTANT`) |
| `cfgPath` | `Q_PROPERTY QString` | the file being edited; Save writes here, Save As re-points it (a `:/` preset is refused as a save target) |
| `loaded` | `Q_PROPERTY bool` | `commandModel.rowCount() > 0` |
| `loading` | `Q_PROPERTY bool` | true while a worker parse is in flight |
| `lastError` | `Q_PROPERTY QString` | empty on success; set from the same checks the widget build ignored |
| `binds` | `Q_PROPERTY QVariantMap` | active binds: lowercase key token → command; notifies on every change, so the caps recolour (D16) |
| `setBind(key, command)` | `Q_INVOKABLE bool` | see `CommandListModel` above; reaches the file on the next save (D16, D20) |
| `clearBind(key)` | `Q_INVOKABLE bool` | remove key token `key`'s bind row(s); false when it had none |
| `dirty` | `Q_PROPERTY bool` | true when any app-owned row exists (NOTIFY `bindsChanged`); the post-save resync clears it until the next edit; the Save action's enable flag |
| `canSave()` / `saveBlockReason()` | `Q_INVOKABLE` | false plus a reason string when saving is impossible (nothing loaded, or the path is a bundled `:/` preset) |
| `needsConfirm()` | `Q_INVOKABLE bool` | true when the file changed on disk, is read-only or has vanished since load; the QML confirm dialog sits between the question and the write |
| `saveCfg()` / `saveCfgAs(path)` | `Q_INVOKABLE bool` | the surgical save (section 4); Save As re-points `cfgPath` at the new file, which becomes the source for later edits. Emits `saved(path)` on success |

## 6. QML rules

- New QML files start `pragma ComponentBehavior: Bound`; delegates declare
  `required property var modelData`. Keep this on new delegates or rendering breaks at
  runtime, not compile time (the keyboard clusters rendered without keys once already).
  Not all existing files carry the pragma yet - 10 of 17 lack it (2026-10-13), which is
  part of the roadmap's QML cleanup.
- Qt 6.12 runtime: `MenuItem` has no `shortcut` property - use `Action`s for menu
  shortcuts (File menu: Ctrl+S / Ctrl+Shift+S).
- `StackView.replace()` keeps `pageStack.depth` at 1 - page identity comes from the
  explicit `editorActive` property in `Main.qml`, not depth. The menu bar hangs its
  visibility on that same flag.
- QML files are embedded in the qrc: **QML edits need an app rebuild to take effect**.
- Console output on Windows needs `QT_FORCE_STDERR_LOGGING=1` to reach stderr.
- Theme: all colours/metrics/fonts are tokens in `Theme.qml` - no hardcoded duplicates
  anywhere in `qml/`. Combo-popup rule: the highlighted row must be *lighter* than the
  popup body (`colorSurfaceRaised` on `colorSurfaceSunken`) - on a dark theme a darker
  highlight reads as "recessed", not "selected".

## 7. Deliberate deviations (D1-D20)

Cite a D-number when deviating further. D15/D16 add capability on purpose; every other
row removes or re-homes something without changing what the app can do.

| # | Deviation | Reason |
|---|---|---|
| D1 | One `ApplicationWindow` with a page stack instead of two `QMainWindow`s swapped by `hide()`/`show()` | The widget build never showed both at once; a single window removes the second hidden top-level and the stale `changeWindow()` dead code. Layout per page is unchanged. |
| D2 | One window size for both screens (1000x500, the start screen's). The start screen is fixed (minimum = maximum); the editor opens at the same size and is resizable from there up | The widget build resized between screens, so a dragged editor size was lost on every reload. `Main.qml` releases the maximums to the desktop size on the page change and keeps the minimums at the shared default. **Known artifact:** Windows sizes a min==max window *inside* the invisible resize borders, so the start window lands at 998x493 and logs one `setGeometry` warning per launch. Qt Quick's `Window` has no `resizable` flag, so min == max is the only way to pin it. |
| D3 | `Go` is enabled exactly when the path is non-empty | Reproduces P11/P7/P10 and fixes the widget's stale-enabled Go. Consequence: clearing the combo also disables Go. |
| D4 | Cap labels normalised (`f3`→`F3`, `Prt\nScn`→`PrtScn`, `KP\nEnter`→`KP Enter`) | A game-style UI labels keys the way the keyboard does. Tokens are untouched. |
| D5 | Tall numpad keys expressed as `rowspan: 2` instead of 130/140 px heights | Same visual intent, ignores the 10 px inconsistency between `+` and `Enter`. |
| D6 | Placeholder tab renamed from the widget's joke string to `Placeholder` | It is a placeholder; the string carries no meaning. |
| D7 | `printCfg()` dump goes through `qInfo()` instead of raw stdout; the key-token `printf` becomes `console.log` | Uses Qt/QML logging; the dump is a diagnostic, so where Qt routes it is not part of the contract. |
| D8 | Design tokens (colors, spacing, radii, fonts) centralised in `Theme.qml` | The widget build hard-coded the fonts in every `.ui`. |
| D9 | `keybform.h`'s `KeyId`/`KEYCODES` move to a QML singleton | The table is presentation data. When the bind engine demands it, it should move to C++ and be injected (roadmap item). |
| D10 | Overview tab shows `No config loaded.` when the list is empty | An empty box cannot be told apart from "the load failed". |
| D11 | Start screen shows the load failure (`Could not load <path> (<reason>)`) in the Go row; `Browse` converts the dialog's `file://` URL to a local path first | The conversion is a platform requirement; surfacing the failure fixes the engine's silently-ignored `Lexer::open` failure. |
| D12 | Short description line under the preset combo; placeholder text in the bind dialog's empty combos/argument area | Empty-state wording. The preset note is data (`presets[].note`), so future presets get one for free. |
| D13 | Main keyboard rows stretched to the widest row (1012 px) with computed per-row gaps | Matches the widget's `QHBoxLayout` behaviour; keeps the main block rectangular. The middle and numpad clusters keep the plain 4 px gap. |
| D14 | The bind dialog opens centred on the window and is dragged by its title bar, clamped inside the window | A stock QML `Dialog` sits at (0,0) and cannot be dragged; the widget dialog could be dragged off-screen. |
| D15 | The keyboard panel scrolls in both directions when the window is too small, and the keyboard alone can be zoomed (`Ctrl`+wheel, `Ctrl`+`+`/`-`/`0`) | The keyboard is 1632 px wide and does not fit the editor; the outer keys were previously unreachable. |
| D16 | A cap whose key the loaded config binds is filled burnt orange; OK in the bind dialog edits the bind | The keyboard was a viewer. The Custom Command text is the command applied (an empty field removes the bind); `Existing Bind:` is display-only. The edit reaches the file through the surgical save (D20); until then it lives in the loaded model. |
| D17 | The window is pinned to the top-left corner of the primary screen at startup | On multi-monitor desktops the OS could open the window straddling two screens. Only the position is fixed. |
| D18 | `setBind` lowercases only the first word of the command; arguments keep the case typed | Console command names are case-insensitive, arguments are not: `say "Hello"` became `say "hello"`. Rows read from a file stay lowercase because `cfgLex` folds every line, so an edit applied twice still yields one identical row. |
| D19 | `\"` and `\\` inside a quoted token no longer end it, and `//` starts a comment only outside a quoted token | Matches the game's tokenizer (section 2). Token text is unchanged for every config that contains neither. |
| D20 | The `File` menu gains working `Save` (Ctrl+S) and `Save As...` (Ctrl+Shift+S) | The first actions in the previously empty menu bar; the other menus stay empty until something belongs in them. |
