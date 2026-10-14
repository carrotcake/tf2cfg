# Design: Config Editor tab (the placeholder tab, non-bind editing)

Status: design, pending decisions (§11). Decisions marked **[user, 2026-10-14]** were made
in conversation; everything else is derived from repo facts cited inline.

## 1. What this tab is

The third tab of the editor (currently `PlaceholderTab.qml`, the widget build's
`"Fuck if I know what should go here"` — `docs/legacy/pickle-inferences.md`) becomes the
**Config Editor**: a two-panel editor for every parsed config line that is not a bind.
Name **[user]**: `Config Editor`. The `EditorPage` tab model becomes
`["Command Overview", "Config Editor", "Edit Binds"]`.

Binds stay owned by Edit Binds. Selecting a `bind`/`bindtoggle` row in the Config Editor
opens a yes/no dialog that redirects to Edit Binds (and opens `BindEditorDialog` for that
key) **[user]**. `unbind`/`unbindall` are ordinary commands edited inline (§7).

The surgical save already anticipated this tab: *"the placeholder tab will set it
[appOwned] on the rows it edits. Both regeneration and deletion fall out of the flag"*
(`docs/legacy/ui-qml-rewrite.md` §10.1 decision 2). Nothing about the save path changes
in principle — the tab is a second producer of app-owned rows.

## 2. Layout and interaction

Master–detail **[user]**:

- **Left panel**: the parsed rows in file order, one selectable entry per source line
  (a `;`-compound line is ONE entry rendered as its joined text, §5). Type badge per row
  (cvar / cmd / alias / comment / not in catalog), catalog doc string as tooltip on known
  rows. A search box and a type filter sit above the list **[user]** — plain substring
  match on the row text, filter by the §3 badges (all types by default). Roles `CommandRole`/`ArgsRole`/`TypeRole`/`TypeNameRole` already exist on
  `CommandListModel` and were added "so the future typed editors do not need this class
  reopened" (`ui-qml-rewrite.md` §6) — this tab is that consumer.
- **Right panel**: the editor for the selection (§4). Nothing selected: an empty-state
  hint, mirroring the Command Overview's "No config loaded." pattern.

Empty configs show the Command Overview's empty-state text. All new controls carry
`Accessible.*` (the accessibility bar from `docs/accessibility.md`); any new color must
reuse existing Theme tokens so `tools/contrast_check.py` stays valid.

## 3. Row taxonomy in the list

| Parsed kind | CmdType | List badge | Editor |
|---|---|---|---|
| cvar assignment | `CMD_IVAR`/`CMD_FVAR`/`CMD_SVAR`/`CMD_BVAR` | cvar | guided value editor (§4.1) |
| plain command | `CMD_CMD` | cmd | guided arg editors (§4.1) |
| alias definition | `CMD_ALIAS` | alias | chain editor (§5) |
| unknown head | `CMD_UNKWN` | not in catalog | raw text (§4.3) |
| comment line | `CMD_COMMENT` (new, §6) | comment | raw text |
| `bind`/`bindtoggle` | parse type | bind | **redirect** to Edit Binds |
| `unbind`/`unbindall` | `CMD_CMD` | cmd | inline, with replay warnings (§7) |

Unknown heads are *not errors*: the game accepts aliases defined in another cfg executed
at runtime, so `config.cfg` may legitimately reference names this file never defines
**[user]**. They render as raw text with a "not in catalog — may be an alias defined in
another config" note.

Whitespace-only lines stay outside the model (they are not rows today and the save copies
them byte-for-byte) **[user: ignored]**.

## 4. The right-panel editor

Two paths, mirroring Build-A-Bind / Custom Command **[user]**:

### 4.1 Guided editing

For catalog-known commands, driven by the metadata the catalog already carries
(`ConsoleCommandModel` roles: `InputRole` usage signature, `DefaultRole`, `MinRole`,
`MaxRole`, `ArgsSpecRole` from argbounds.csv, `DocsRole`, `CheatRole`).

- **cvar rows**: one value field typed by the catalog — bool → checkbox, int/flt →
  spinbox clamped to the catalog's min/max (one-sided bounds respected), str → line edit
  with the catalog default as placeholder.
- **cmd rows**: one editor per token of the usage signature (`parseSignature` logic
  already exists QML-side in `BindEditorDialog`); optional `<...>` tokens get an include
  checkbox.
- **No bind-mode radio here.** Toggle-vs-incrementvar is a *bind* concern
  (`allcmds-v5-migration.md` §4.4); the Config Editor sets values directly
  (`cl_interp 0.033`), so plain value fields only.
- A command whose signature is `none` shows the static "takes no arguments" note
  (the same stub text the bind dialog uses).

### 4.2 Custom text entry

A monospaced text field with the row's current `LineRole` text (the Command Overview
rendering — one rendering, not two, per §10.1). On commit (or live, see §8) the backend
parses the line and the guided fields populate from the result **[user]**.

### 4.3 Raw rows

`CMD_UNKWN` and comment rows get raw text only — no guided fields, because the catalog
has no signature to shape them.

## 5. Compound lines and the chain editor

A segment-list editor: each segment is a command editor (guided or custom, §4), with
add / remove / reorder. The rendered line joins segments with `"; "` — the exact
separator the compound-line save rebuild already emits (`configcontroller.cpp`
`writeConfig`). Three consumers:

1. **Parsed compound lines** (`a; b; c`): the parser groups rows by `sourceLine`, and the
   save's decided behavior is "always rebuild touched compound lines"
   (`docs/DESIGN-should-fixes.md` #2). The chain editor edits the group as one unit.
2. **Alias bodies**: `alias name "c1; c2"` parses as *one* quoted token; the chain editor
   splits the *body* into segments using the same quote-aware tokenizing, so an alias is
   creatable, parseable and editable as a chain **[user]**. Writing back re-quotes the
   body, honouring the `\"`/`\\` rules that `quoted()`/`unquoted()` own (D19).
3. **BindEditorDialog**: a third tab, **"Alias"**, handling all alias logic **[user]**:
   alias name field (existing alias names from the parsed `CMD_ALIAS` rows prefill the
   combo) + the chain editor for the body. Committing writes BOTH rows: the
   `alias name "body"` definition (a new `CommandListModel::setAlias(name, body)` — an
   app-owned session row anchored after the last alias line, so definitions stay
   together and are defined before use) and the `bind "<key>" "<name>"` row the dialog
   always writes. The shared segment editor (consumers 1–2) is the same component.

Aliased names: the parser registers alias names it sees in *this* file (`gameconfig.cpp`
`appendCfg` inserts into `cmd_types`), so a later row bound to them types as
`CMD_ALIAS`; names from other cfgs stay `CMD_UNKWN` (§3).

## 6. Comments (the engine chunk)

Today `cfgLex::preprocess` truncates the line at the first unquoted `//` *before*
tokenizing, so comment text never reaches the model and the save copies comment lines
byte-for-byte as "content the app does not own". Comment editing **[user]** needs:

1. **Capture**: `preprocess` splits the tail off and keeps it (per-line side channel in
   the lexer, like `currentLine()`); the case fold must apply only to the command part,
   never to comment text.
2. **Model**: new `CmdType CMD_COMMENT`; a standalone comment line becomes an Entry whose
   args hold the raw comment text (so `args.join(" ")` re-renders it verbatim and the
   Command Overview keeps showing every line); a trailing comment becomes a
   `commentTail` member on the owning row, re-emitted as `cmd text // tail` when that
   line is rebuilt.
3. **Save**: comment rows and tails participate in ownership like any row — edit sets
   `appOwned`, delete drops the line, unowned lines still copy verbatim. The
   resync-after-save re-parse reproduces both forms.
4. **Overview parity**: the Command Overview currently shows only command rows; comment
   rows appearing there is accepted as part of this design (the list gains a muted
   comment badge).

Editing comments is deferred to Chunk D (§10) because it touches the lossless-parse
foundation; everything else ships first.

## 7. Warnings from replay order

The binds replay in `scanBinds()` (`CommandListModel`) already walks rows the way TF2
does — later rows win. The same walk produces warning flags, exposed as a new
`WarningRole` (string, empty when clean):

- **Dead bind**: `unbind "k"` (or `unbindall`) appearing *after* a `bind`/`bindtoggle`
  of the affected key(s) — the unbind wins and the earlier bind is dead **[user]**.
- **Overridden cvar** **[user]**: a cvar set again later in the file — the earlier row
  never takes effect. Same warning type and rendering as the dead-bind case.

Both render as a muted warning badge in the list and a note in the editor. Warning text
cites the winning line ("overridden by line 214"), mirroring how the disk-change confirm
names its problem.

## 8. Reverse-parse API

Custom text → guided fields needs one authoritative tokenizer; a second quote-aware
parser in QML would drift from `cfgLex`. New C++ entry point on `ConfigController`:

```
Q_INVOKABLE QVariantMap parseCommandLine(const QString &line)
// → { ok, command, args: [..], typeName, known, error }
```

Implementation: run the line through `cfgLex`'s tokenizer on an in-memory buffer (the
lexer class gains a parse-from-string path next to `open()`), classify the head via the
catalog, and let the QML guided editor map args onto fields with the existing
`ArgsSpecRole`/signature logic. The bind dialog's QML-side `splitArgs` stays as-is; new
code does not grow a third tokenizer.

## 9. Save integration (all free)

Every edit/delete sets `appOwned` (and `sourceLine` stays put for parsed rows); new rows
are session rows. The existing machinery then handles everything: owned-line
regeneration as `args.join(" ")`, compound rebuild, `.bak`, disk-change confirm, insert
position, resync-after-save (`configcontroller.cpp`, `ui-qml-rewrite.md` §10.1).

One save-side change is required: the session-row insert point is currently "after the
last bind line" (bind-tuned). Config Editor rows insert **after the currently selected
row** **[user]**, so a session row gains an anchor (`insertAfterRow`, resolved to a file
line the same way `setBind` resolves its own insertions). Rows with no selection anchor
land at end of file. Binds keep their current anchoring unchanged.

## 10. Phasing

| Chunk | Content | Gate |
|---|---|---|
| A | Tab shell + list (badges, tooltips, search + filter, redirect dialog) + warnings (§7) + raw text editing + delete + insert-after-selected + save plumbing | engine tests: round-trip, delete, insert anchor, warning flags |
| B | Guided editors + `parseCommandLine` reverse-parse | tests: parseCommandLine vs lexer truth, field mapping |
| C | Chain editor + alias create/edit + BindEditorDialog "Alias" tab (`setAlias`) | tests: alias body round-trip, compound rebuild |
| D | Comment rows (lexer capture, model, save, Overview) | tests: standalone + trailing comments round-trip |

Chunk A landed 2026-10-14 (`0d9360c`), Chunk B landed 2026-10-14 (guided pane + the
`findCommand` full-record extension; field mapping is QML-side, so its engine gate is
the `findCommandCarriesTheFullRecord` record test and the same-correctness of
`parseCommandLine`).

Each chunk commits separately; `qml` additions carry
`pragma ComponentBehavior: Bound` (AGENTS.md bar) and no new qmllint advisories.

## 11. Resolved decisions (user, 2026-10-14)

1. **Overridden-cvar warning**: yes — flagged exactly like the dead-bind warning; same
   type of warning, one shared replay walk (§7).
2. **Trailing comments**: full support — `commentTail` member + render rule in Chunk D
   (§6).
3. **Bind dialog**: third tab **"Alias"**, carrying all alias logic (§5.3).
4. **Search + type filter** above the left panel: yes, ships in Chunk A (§2).