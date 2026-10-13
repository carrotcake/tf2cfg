# What is this application? — inferences from the repo

Analyzed: 2026-10-12, at commit `8004adf`.
Method: source, `.ui` files, bundled assets, generated `uic` output, and full git history. Every claim below is tied to evidence; speculation is labelled.

## Verdict

**TF2 Config Manager** — a Windows desktop GUI for viewing and editing Team Fortress 2 `.cfg` files (primarily `config.cfg`) without hand-editing text.

The defining idea is that binds are edited **spatially on a picture of a keyboard**: you click a key cap, and a dialog helps you construct a valid `bind` line for it. A bundled dump of every TF2 console command and cvar (`allcmds.csv`, 3767 entries) supplies the vocabulary and the validation metadata (type, default, min, max, flags).

Working title is explicit in the UI: `startwindow.ui:72` → `"TF2 Config Manager"`, `editwindow.ui:20` → `"Team Fortress 2 Config Editor"`.

Confidence: **high**. Evidence is direct strings and object names, not inference from structure alone.

## Intended user flow

| Step | What the user does | Evidence |
|---|---|---|
| 1 | Opens a cfg, or picks a bundled preset | `startwindow.ui:312` "Open your TF2 Configuration File...", `:166` "... Or Choose From a Preset", `:361` placeholder `"< Path To TF2 config.cfg >"` |
| 2 | Presses "Load Selected Config" | `startwindow.ui:423`, handler `startwindow.cpp:50` |
| 3 | Lands in the editor with tabs | `editwindow.ui:25-162`: "Command Overview", a placeholder tab, "Edit Binds" |
| 4 | Browses the parsed commands as text lines | `editwindow.ui:74` `listWidget` in "Liberation Mono", filled at `editwindow.cpp:20-22` with `cmd.args.join(" ")` |
| 5 | Clicks a key cap on the keyboard drawing | `editwindow.cpp:49-55` slot `on_keybWidget_keybButtonClicked` |
| 6 | Builds or types a command for that key | `bindeditdialog.ui`: "Build-A-Bind" and "Custom Command" tabs |
| 7 | *(missing)* Confirms and saves a cfg | no write path exists anywhere in the tree |

Step 7 does not exist yet. The application is currently **read-only**: the only file dialog is `QFileDialog::getOpenFileName` (`startwindow.cpp:16`). No `QFile::open(WriteOnly)`, no serializer, no save action.

## Component intent

### StartWindow — file chooser

Two side-by-side paths, matching the QML prototype's layout (`SSBox` pair with a separator, `StartPage.qml` from commit `9eade33^`).

`startwindow.h:31` holds only two presets:

```cpp
inline static const QString CFG_STRINGS[2] = {":/cfg/b4nny.cfg", ":/cfg/default.cfg"};
```

but four cfgs are packaged in `res/res.qrc`. Presets are literal labels from a player's config pack — `startwindow.ui:237` `"b4nny (config.cfg from b4nny.zip)"`, `:242` `"Stock TF2 config.cfg"`.

The `b4nny_*` split is not guesswork — the three files are exact decompositions of one another:

| File | Content | Confidence |
|---|---|---|
| `b4nny.cfg` | complete recording of that player's `config.cfg` (66 binds + settings) | high |
| `b4nny_defbinds.cfg` | b4nny's settings + stock default binds — its 54 `bind` lines are byte-identical (after whitespace normalization) to `default.cfg`'s 54 | high |
| `b4nny_nobinds.cfg` | b4nny's settings only, zero `bind` lines, no `unbindall`; its 206 non-blank lines are byte-identical to the settings half of `b4nny_defbinds.cfg` | high |

Three of the four start with `unbindall`, standard TF2 cfg practice, consistent with "the presets replace the user's current binds"; `b4nny_nobinds.cfg` deliberately omits it because it carries no binds.

### EditWindow — tabbed editor

- **Command Overview**: dumps every parsed entry, one per line, monospaced. This is scaffolding, not a designed feature — `GameCfg::printCfg()` (`gameconfig.cpp:96`) does the same via `printf`, and the tab holds a leftover Designer item, `editwindow.ui:92` `"test_item asdf"`.
- **Third tab**: `editwindow.ui:136` `"Fuck if I know what should go here"`. Genuinely unwritten.
- **Edit Binds**: hosts `KeybForm` as a promoted custom widget (`editwindow.ui:282-292`), and the header path in the Designer metadata, `src/ui/keybform.h`, confirms it is the project's own component rather than a stock one.
- **Menu bar**: `File`, `Edit`, `View`, `Help` exist with **no actions at all** (`editwindow.ui:255-278`). This is where a Save/Save As would be expected to land.
- **Prev / Next**: tab navigation (`editwindow.cpp:41-47`), currently the only "paging" the app has.

### KeybForm — the keyboard drawing

A fixed 1560×480 form (`keybform.ui:9-19`) inside a `QScrollArea`, holding **104 `QPushButton` key caps**, all in the TF2 Build font with `PointingHandCursor`. **100 are 64×64**; the four exceptions are shaped like the real keys they depict:

| Cap | Size | Note |
|---|---|---|
| `btn_bkspc` | 128×64 | wide, like a real Backspace |
| `btn_kp0` | min 130×64, max 128×64 | min width exceeds max width, so Qt clamps to 128 — a small Designer slip, not intentional |
| `btn_kpplus`, `btn_kpenter` | 64×130, 64×140 (max only) | tall, spanning two numpad rows |

Design decisions visible in the data:

| Observation | Evidence | Implication |
|---|---|---|
| 102 keys are bindable | 102 `X()` calls in `keybform.cpp:26-134`, 102 `KEYCODES` entries, `KEYCOUNT` = 102 | full US layout incl. numpad and F-row |
| 2 buttons deliberately inert | `btn_prtsc_DISABLED`, `btn_menu_DISABLED` — `enabled=false` plus tooltip `"This key cannot be bound to an action."` | faithful to TF2, where these two cannot be bound |
| Grouped, non-exclusive | generated `ui_keybform.h`: `keybButtons = new QButtonGroup(KeybForm); keybButtons->setExclusive(false);` | caps behave as click targets, not a radio selection |
| Enum ↔ string mapping | `keybform.h:119-220` `KEYCODES[]` maps enum id → TF2 key token (`"scrollock"`, `"kp_slash"`, `"uparrow"`) | the strings are exactly what belongs inside `bind "<key>"` |
| Click → signal → dialog | `keybform.cpp:11-15` emits `keybButtonClicked(id)`; `editwindow.cpp:49-55` opens `BindEditDialog` prefilled with `bind "<key>"` | the keyboard's whole purpose is bind editing |
| No state on the caps | no `setChecked`, no per-key text update anywhere | binds from the loaded cfg are **not** yet reflected on the keyboard — a core unfinished piece (the later QML rewrite landed this as D16 in `docs/legacy/ui-qml-rewrite.md`) |

Spelling inside `KEYCODES` is copied from the game's own cvar table, typos included (`"scrollock"`, `"KP_home"`, `"kp_5"`); these must match the engine verbatim, so they are not bugs. Note `"shift"` / `"rshift"`, `"ctrl"` / `"rctrl"` mirror TF2's asymmetric naming.

### BindEditDialog — the bind builder

Two ways to author a bind, matching how TF2 players think about it:

1. **Build-A-Bind** (`bindeditdialog.ui:100`): `Category:` combo, `Command:` combo, `Arguments:` area. Intent: pick a command from the known-command database, and get type-appropriate argument input (an int cvar with a range becomes a bounded spinbox, a string cvar a line edit, a `cmd` no argument space).
2. **Custom Command** (`bindeditdialog.ui:213`): a `QPlainTextEdit` in Liberation Mono with placeholder `"< command goes here >"` — free-form escape hatch for anything the database does not cover.

Both paths are empty: no items are added to either combo, and the arguments `QWidget` (`bindeditdialog.ui:205`) is a bare native placeholder. The dialog is entirely `accept()`/`reject()` wiring from Designer (`bindeditdialog.ui:308-340`) plus an empty-accept slot (`bindeditdialog.cpp:18`). Nothing reads or writes a result back to the caller — `exec()` is called at `editwindow.cpp:55` and the return value is discarded.

"Category" has no obvious source column in `allcmds.csv` (columns are name, type, default, min, max, flags, doc). It is probably meant to be a hand-curated grouping, or derived from the flags column (`client`, `archive`, `cheat`, `game`, `replicated`, ...). Confidence: **low** on which.

### GameCfg / Lexers — the engine

The `src/gamecfg` module models the webs of a real cfg file:

| Type | Purpose | Evidence |
|---|---|---|
| `cmd::Entry` | one parsed line + its classification | `convars.h:11-16` |
| `cmd::ConVar` → `IntVar`/`FltVar`/`StrVar`/`ConCmd` | type-specific metadata, incl. min/max ranges | `convars.h:17-111` |
| `GameCfg` | owns parsed entries, the cvar type map, and the name→index map | `gameconfig.h:7-27` |
| `Lexers::cfgLex` | tokenizes `.cfg` files | `lexers.h:49-98` |
| `Lexers::cmdLex` | tokenizes the `allcmds.csv` database | `lexers.h:100-170` |

That the engine distinguishes int/float/string cvars *and* carries `Range{min,max}` while nothing in the UI consumes either is the clearest signal of the intended finished product: **validated, type-aware editing**. Ranges are read out of the CSV explicitly (`gameconfig.cpp:64-70,78-83`) and stored; `getMinVal`/`getMaxVal`/`getDefaultVal` are defined (`convars.h:47-50,75-78`) and have **zero callers**. (Two different `hasRange` methods exist — `convars.h:50` on cvars, `lexers.h:130` on CSV tokens; only the lexer one is called.)

The engine also already tries to track **aliases** (`gameconfig.cpp:23-25` tags a name as `CMD_ALIAS` when it follows `alias`), which implies an intended feature: letting a key be bound to a `+jump;+duck` style alias chain rather than a single command. No UI reads `CMD_ALIAS` yet.

`cvar.cpp` (`src/gamecfg/cvar.cpp`) is a one-line stub that only includes the header — an abandoned split of `GameCfg` out of `gameconfig.cpp`.

## Bundled assets

| Asset | Role | Confidence |
|---|---|---|
| `res/cfg/allcmds.csv` (3767 rows, 7 columns) | TF2 console command / cvar database: name, `cmd`/`ncmd`/empty, default, min, max, flags, description | high — the parser's field list matches exactly (`lexers.h:107-118`) |
| `res/cfg/default.cfg` | stock TF2 binds, the "Stock TF2 config.cfg" preset | high |
| `res/cfg/b4nny.cfg`, `b4nny_defbinds.cfg`, `b4nny_nobinds.cfg` | a pro player's config as presets, in the three decompositions described above | high |
| `res/font/tf2build.ttf`, `tf2professor.ttf`, `tf2secondary.ttf` | TF2's own UI fonts, loaded at startup (`main.cpp:11-14`), used throughout the `.ui` files | high — deliberate art direction, not decoration |
| `res/font/LiberationMono-Regular.ttf` | added later (commit `5254da9`) specifically for the monospaced cfg-listing widget (`editwindow.ui:75-79`, `bindeditdialog.ui:256-261`) | high |

The font choices say the target audience is TF2 players who will recognize the game's own typeface, and the mono font says "this is code, show it as code".

## Origin and history

Read from git, not guessed:

- The project began on **2024-05-08** as a **Qt Quick / QML prototype**: `Main.qml`, `StartPage.qml`, `TopBar.qml`, plus reusable components `SSBox.qml` / `SSBText.qml`, built with **CMake** (`CMakeLists.txt`), running under Qt 5.15 (the earliest keyboard header was generated by `uic 5.15.13`).
- The QML version's dark theme colors survive conceptually but the files are gone: `Main.qml` deleted (`3a422db`), `StartPage.qml` (`9eade33`), `TopBar.qml` (`bd28d4b`) — the latter carrying a literal `/* TODO: menu bar & tab selection go here */`.
- It was then **rewritten as Qt Widgets with qmake**: `ui/QWidget/keyboard.ui` (3196 lines, `uic` 5.15.13 era) → `ui/keyb.ui` → `src/ui/keybform.ui`, growing to 5557 lines by the time the fonts were packaged (`cf2b9a9`) and 5707 lines at `cfe8d64` ("hard code everything to piss off the math people", +229/−79) — individual 64×64 key caps replacing grouped rows. Unchanged since.
- `keyb.h` (2025 lines, a committed `uic` output) was deleted when the layout was reorganized (`753ed8d`).
- Presets were renamed from `b4nny_config.cfg` / `config_default.cfg` to today's names (`cf2b9a9`).
- Upstream is `https://github.com/carrotcake/tf2cfg.git`; the build target is called `tf2cfgreal` because the `.pro` filename doubles as `TARGET`.

The QML names are informative: `SSBox` is a rounded dark rectangle and `SSBText` a centered white label, used to build the two-panel start page. The start window today is a 1:1 widget translation of that layout.

## What is unfinished, ranked by how much it blocks the verdict

1. **No save path.** No serializer, no write dialog, no menu action. The app cannot produce a cfg. Everything else is in service of this missing feature.
2. **Binds never reach the keyboard.** `CMD_BIND` exists in the enum (`convars.h:9`) but is never assigned anywhere — the only values ever inserted into `cmd_types` are `CMD_CMD`, `CMD_IVAR`, `CMD_FVAR`, `CMD_SVAR` and `CMD_ALIAS`, so `cmdToType` can never report a bind. No loaded bind is ever painted onto a key cap. (The later QML rewrite paints the caps from the parsed rows instead, D16 in `docs/legacy/ui-qml-rewrite.md`; the engine gap described here still stands.)
3. **BindEditDialog does nothing.** Empty combos, placeholder argument area, no result plumbing.
4. **Parse fidelity is not yet good enough to trust for editing.** `Lexer::open` lowercases the entire file and truncates lines at the first `//` (`lexers.h:19-23`), quotes are retained inside tokens (`configlexer.cpp:27-32`), and `allcmds.csv` indexing is unchecked (`cmdlexer.cpp:29-37`). Details in `docs/CODE_REVIEW.md`, findings 1-3 and the Info section.
5. **Two presets bundled but unreachable**; the placeholder third tab; the empty menu bar.

## Confidence summary

| Inference | Confidence |
|---|---|
| It is a TF2 cfg viewer/editor | high — literal UI strings |
| Keyboard exists to pick a key to bind | high — click handler prefills `bind "<key>"` |
| `allcmds.csv` is the command/cvar reference database | high — parser fields match columns exactly |
| Intended to validate edits using type + min/max | medium-high — metadata parsed and stored, no consumer yet |
| Intended to write cfgs back to disk | medium-high — implied by every other feature, but no code exists |
| "Category" means a curated command grouping | low — no source data found |
| `b4nny_defbinds` / `b4nny_nobinds` are preset variants (settings+binds vs settings only) | high — verified byte-identical decomposition, but no code exposes them |
| Aliases are meant to be first-class (bind a key to a chain) | medium — alias tracking exists in the parser only |
| The QML prototype was abandoned for Widgets rather than in parallel | high — QML files deleted in the same era the widget UI grew |

## Explicit speculation

Not supported by evidence in this repo, listed so it is not mistaken for fact:

- Whether the finished app intends to edit `class_*.cfg`, `autoexec.cfg`, or only `config.cfg`. The UI says "config.cfg" but the parser is generic.
- Whether it aims to write into the TF2 install directory (`tf/cfg/`) via a directory picker, as opposed to "Save As" to an arbitrary path.
- Whether the keyboard was ever meant to show key *labels* per bind (e.g. `+jump`) rather than only use caps as targets.
- Whether "Category" was to come from a newer, richer command database than the bundled `allcmds.csv`.
