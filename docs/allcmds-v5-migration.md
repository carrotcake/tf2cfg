# Migrating the parsing system to allcmds v5 (10-column format)

Version 5 of `allcmds.csv` adds three columns (input type, platform, context) to the
previous seven. This doc lists every change the parser and the Build-A-Bind UI need.
Field meanings live in [res/cfg/schema.md](../res/cfg/schema.md), flag/tag vocabulary
in [res/cfg/flags.md](../res/cfg/flags.md).

The file is now canonical as `res/cfg/allcmds.csv` (renamed from `allcmds_v5.csv`);
older revisions remain as `res/cfg/allcmds_*_legacy.csv` and are **not** in the
resource file.

## 1. Immediate fix (done with this doc)

- `res/res.qrc` referenced `cfg/allcmds.csv`, which the versioning renames had removed
  — the resource build was broken. Resolved by renaming `allcmds_v5.csv` back to
  `allcmds.csv`. `GameCfg::loadAllCmds()`'s path `"://cfg/allcmds.csv"` stays valid.
  Future data refreshes must keep the canonical name and add a new `_legacy` file.

## 2. Parser changes (`src/gamecfg/lex/`)

`cmdLex` hardcodes the 7-column layout: its `Token` has six strings (name, is_cmd,
def, min, max, docs) and `cmdlexer.cpp` builds the token as `Token(arr[0..4], temp)`,
where `arr[5]` (the flags field) is silently discarded.

Required changes in `lexers.h` / `cmdlexer.cpp`:

1. **Extend `Token` to carry all 10 fields.** Add `flags`, `inputType`, `platform`,
   `context` plus accessors. In `cmdlexer.cpp` (`cmdLex::nextToken`), the `STATE_END`
   case currently returns before the final field is pushed to `arr`; push `temp`
   first and construct from `arr[0..9]`. The existing DFA handles the quoted fields
   correctly — do not touch the state table.
2. **Stop discarding flags.** `Token` never exposed column 6; expose it now. The
   `cheat` flag gates Build-A-Bind argument generation (section 4) and future
   permission-aware features.
3. **Fix the `//` comment chop for this file.** `Lexer::open()` truncates every line
   at the first `//`. That is correct for `.cfg` files, but it corrupts descriptions
   containing URLs — `replay_fileserver_path`/`replay_fileserver_port` lose
   everything after `http:`. Move the chop out of the shared `Lexer::open()` into
   `cfgLex` (config files only), or skip it when the `//` follows a `:`.
4. **Stop lowercasing for `cmdLex`.** `open()` lowercases every line, which mangles
   description prose (`BLU`/`RED` team names, file paths, capitalized modes) and was
   only ever needed because config files are case-insensitive. Keep it for `cfgLex`;
   for `cmdLex` preserve the original text and let consumers (the `cmd_map` hash,
   `CommandListModel`) compare case-insensitively instead.
5. **Remove the dead debug block** in `cmdlexer.cpp` (`if (line > 2099) { int a = 2+3; }`)
   and the local `QStringList arr;` that shadows `Lexer::arr` — both leftovers.

## 3. Model changes (`src/gamecfg/gameconfig.cpp`, `convars.h`)

`GameCfg::loadAllCmds()` currently guesses a cvar's type from its default value
(`toLongLong` then `toDouble`, else string). v5 makes this exact — column 8 states
the input type. Also: cvars with an **empty default are dropped entirely** today
(`hasDefault()` gate), which silently hides ~60 string cvars (`cl_crosshair_file`,
`cc_lang`, ...).

1. **Add `BoolVar`** to `convars.h` (subclass of `ConVar`, `CmdType CMD_BVAR`,
   default 0/1, no range) — decided: bool cvars get first-class representation,
   not `IntVar` reuse. Extend `typeName()` in `commandlistmodel.cpp` with `bool`.
2. **Switch type selection to the input-type field**: `bool`→`BoolVar`,
   `int`→`IntVar`, `flt`→`FltVar`, `str`→`StrVar`. Delete the guessing ladder.
3. **Accept empty defaults**: create `StrVar("")` when the input type is `str` even
   if column 3 is empty (instead of skipping the row).
4. **Fill `ConCmd::usage` from column 8** (the `[type]`/`<type>` signature) instead
   of the hard-coded `""`; `hasUsage()` can then check non-empty usage. The signature
   grammar: space-separated tokens, `[t]` mandatory, `<t>` optional, `t ∈ {bool,int,
   flt,str}`, or `none`.
5. Store platform/context on `ConVar` (two new `QString` members with getters) — the
   UI filters on them; keeping them on the model avoids a second CSV pass.
6. `ncmd` rows: column 2 is `ncmd`, not empty, so `isCmd()` already treats them as
   commands. Keep that; tag them with their context/platform like any command.

## 4. Build-A-Bind (`qml/dialogs/BindEditorDialog.qml`, keyboard UI)

The Build-A-Bind tab ("Category" + "Command" comboboxes, arguments pane) was an empty
placeholder (deferred as a future feature in docs/ui-qml-rewrite.md, P33/D16). The
new columns are exactly the data it needs. Decisions below were confirmed with the
project owner.

### 4.1 Data plumbing

- New QML-facing model (e.g. `ConsoleCommandModel`, a `QAbstractListModel` built from
  `GameCfg::known_cmds`) with roles: `name`, `docs`, `inputType`, `platform`,
  `context`, `cheat`, `min`/`max` (ranged cvars). `CommandListModel` stays as the
  model for *parsed config lines* — the two must not merge.

### 4.2 Category and platform dropdowns

- **Category** = column 10 (context): `client controls`, `client settings`,
  `client render`, `server commands`, `server settings`, `engine` — plus an implicit
  `all`. Default: `client controls` (that is what key binds are for).
- **Platform** = column 9, user-selectable dropdown: `universal` (default),
  `linux`, `windows`, `xbox`, `all`. The `xbox` entries are inert on PC and stay
  hidden unless explicitly selected.

### 4.3 Command list

- Combobox lists commands in the selected category/platform, sorted by name; the
  description goes in the item's tooltip. Cheat-flagged commands stay listed (the
  tooltip shows the `cheat` flag) — flagging is a display concern, not a filter.

### 4.4 Arguments pane (replaces the "Nothing to enter yet" stub)

Generated generically from the input-type signature:

| Signature | Editor |
|-----------|--------|
| `none` | Pane disabled with a static "takes no arguments" note |
| `bool` | Toggle checkbox **plus a bind-mode radio** (decided): *Toggle* or *Fixed value* |
| `int` / `flt` | Spin box, bounded by min/max when the CSV has them, plus the same *Toggle* / *Fixed value* radio; toggle adds a step field (default 1 / 0.1) |
| `str` | Line edit, placeholder shows the CSV default |
| compound | One editor per token, in order; `<...>` optional tokens get an include checkbox |

Bind-mode is a per-bind user choice (decided). Generated text:

- command: `name arg1 arg2 ...` (missing optional args omitted)
- cvar, fixed value: `bind "k" "cvar value"`
- cvar, toggle: `bind "k" "incrementvar cvar <min> <max> <step>"` — for bool that
  reduces to `bind "k" "incrementvar cvar 0 1 1"`, matching TF2's own `BindToggle`.
  Note the engine command is spelled `incrementvar`, no space.

The pane preview line and the dialog's OK path (`cfg.setBind`) are unchanged — the
generated string lands in the same `Custom Command` buffer, keeping the two tabs
symmetrical.

## 5. Work items checklist

- [x] `lexers.h`: 10-field `cmdLex::Token`, new accessors
- [x] `cmdlexer.cpp`: push last field on `STATE_END`; remove debug block + shadowing
- [x] `lexers.h`: comment-chop and lowercasing scoped to `cfgLex`
- [x] `convars.h`: `BoolVar`/`CMD_BVAR`, platform/context on `ConVar`
- [x] `gameconfig.cpp`: type from column 8, empty-default strings kept, usage filled
- [x] `commandlistmodel.cpp`: `typeName()` gains `bool`
- [x] new `ConsoleCommandModel` (C++ side of 4.1)
- [x] `BindEditorDialog.qml`: dropdowns, filtered command list, generated arguments pane
- [ ] optional: surface new columns in `CommandOverviewTab`

## 6. Known data caveats (documented, not blockers)

- Command signatures default to `none` when the parameter list was never publicly
  documented; the console's own autocomplete is the ground truth for those.
- The `bool` vs `int` boundary is heuristic for 0/1 defaults without bounds; the
  `INPUT_OVERRIDES` list in `tools/field_dicts.py` records the manual fixes made so far.
- The `linux` platform tag derives from dump provenance (a Linux client); a few
  `gl_*` cvars may also exist on the Windows OpenGL renderer. Treat `linux` as
  "seen in Linux dumps", not "proven absent elsewhere".
