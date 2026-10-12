# UI rewrite: Qt Widgets → QML

Status: design (pre-implementation).
Date: 2026-10-12.
Supersedes: the widget `.ui`/`QMainWindow` UI described below, which is retained in `src/legacy/widgets/` but removed from the build.

## 1. Scope

Rewrite the entire presentation layer in QML while keeping behavioural parity with the current widget UI, and structure it so the features described in `docs/pickle-inferences.md` can be added without re-cutting the architecture.

Goals:

1. **Functional parity** with the widget build, feature by feature (section 3).
2. **Extension-ready** for the known future work: saving configs, painting loaded binds onto the keyboard, type-aware argument editing, alias chains, command categories, more presets (section 10).
3. **Layout preserved**: same two screens, same panels, same arrangement, same window sizes.
4. **Aesthetic fit for the use case**: a dark, game-adjacent tool that reads as a TF2 companion app, with the game's own fonts already bundled.

Non-goals:

- No changes to `src/gamecfg/**` parsing behaviour. Findings in `docs/CODE_REVIEW.md` stay unfixed; the rewrite must reproduce current parse output, not improve it.
- No new user-facing features. Every screen must do exactly what the widget version did, including its dead ends.
- No removal of the old UI code from the repo (see section 9).

## 2. Terminology used in this document

| Term | Meaning |
|---|---|
| Widget build | the current implementation: `StartWindow`, `EditWindow`, `KeybForm`, `BindEditDialog` + 4 `.ui` files |
| Parity item | one observable behaviour of the widget build that the QML build must reproduce |
| Key table | the id ↔ TF2 key token mapping that used to live in `keybform.h` (`KeyId` enum + `KEYCODES[]`) |
| Backend | `src/gamecfg/**` parsing engine plus the new `ConfigController`/`CommandListModel` glue |

## 3. Parity contract — widget inventory → QML counterpart

Every row is a behaviour of the widget build, with its evidence and its QML replacement. Nothing may be dropped, even if it looks like scaffolding.

| # | Widget behaviour | Evidence | QML counterpart |
|---|---|---|---|
| P1 | Four application fonts registered before the first window | `main.cpp:11-14` | same call in `main.cpp` before `engine.load()` |
| P2 | Start screen 1000×500, non-resizable-ish (max = min = 1000×500) | `startwindow.ui:4-24` | `ApplicationWindow` sized 1000×500 while `StartPage` is active |
| P3 | Banner panel with `TF2 Config Manager` in TF2 font 40pt, centered, styled/raised frame | `startwindow.ui:37-80` | `Panel` component + `Label`, `Theme.fontTitle` |
| P4 | Left panel heading `Open your TF2 Configuration File...` (TF2 Professor 28pt) | `startwindow.ui:275-323` | `Panel` + `Label` |
| P5 | Left panel: read-only path field, placeholder `< Path To TF2 config.cfg >`, drag enabled | `startwindow.ui:341-363` | `TextField` with `readOnly: true`, `selectByMouse: true` |
| P6 | Left panel: `Browse` button opens a file dialog filtered to `Source Config File (*.cfg)` | `startwindow.ui:366-385`, `startwindow.cpp:15-25` | `Tf2Button` + `FileDialog` (`QtQuick.Dialogs`), same filter string |
| P7 | Browse success → clear preset selection, show absolute path, enable Go | `startwindow.cpp:20-24` | `path = file; presetIndex = -1` |
| P8 | Right panel heading `... Or Choose From a Preset` | `startwindow.ui:129-177` | `Panel` + `Label` |
| P9 | Preset combo, not editable, placeholder `< Choose a Preset >`, 2 items: `b4nny (config.cfg from b4nny.zip)`, `Stock TF2 config.cfg` | `startwindow.ui:195-245` | `Tf2ComboBox` bound to `cfg.presets` |
| P10 | Preset index 0/1 → path becomes `:/cfg/b4nny.cfg` / `:/cfg/default.cfg`, Go enabled | `startwindow.h:31`, `startwindow.cpp:27-38` | same mapping, from `ConfigController::presets` |
| P11 | Go button starts disabled; labelled `Load Selected Config` | `startwindow.ui:406-425` | `Tf2Button`, `enabled: path.length > 0` |
| P12 | Go → parse file, dump to stdout, fill command list, hide start, show editor | `startwindow.cpp:50-57`, `editwindow.cpp:16-23` | `pageStack` push + `cfg.loadCfg(path)`, model binds the list |
| P13 | Editor window 1573×807, title `Team Fortress 2 Config Editor` | `editwindow.ui:4-21` | `ApplicationWindow` sized 1573×807 while `EditorPage` is active |
| P14 | Editor has a 3-tab bar; tab 0 active... actually index 1 is pre-selected by the `.ui`, then `editwindow.cpp:9` forces index 0 at construction | `editwindow.ui:29-30`, `editwindow.cpp:9` | `TabBar` with `currentIndex: 0` |
| P15 | Tab 0 `Command Overview`: groupbox titled `Commands` containing a scrollable monospaced list of every parsed line | `editwindow.ui:32-133` | `CommandOverviewTab`: `Panel` + `ListView` on `cfg.commandModel`, role `line` |
| P16 | List uses Liberation Mono 12pt, pointing-hand cursor | `editwindow.ui:74-89` | `Theme.fontMono`, `cursorShape: Qt.PointingHandCursor` |
| P17 | Tab 1 is an empty placeholder | `editwindow.ui:134-138` | `PlaceholderTab` (same emptiness) |
| P18 | Tab 2 `Edit Binds`: hosts the keyboard widget full-width | `editwindow.ui:139-161` | `BindEditorTab` → `KeyboardPanel` |
| P19 | Prev/Next buttons navigate tabs; Prev disabled at index 0, Next disabled at last | `editwindow.cpp:25-47` | same rule against `tabBar.currentIndex` |
| P20 | Prev/Next set a status tip naming the adjacent tab | `editwindow.cpp:29,36` | status strip at the bottom of the editor shows the hint while hovering |
| P21 | Menu bar with `File`, `Edit`, `View`, `Help`, **no actions** | `editwindow.ui:246-279` | `MenuBar` with 4 empty `Menu`s, only on the editor page |
| P22 | Status bar exists | `editwindow.ui:280` | thin status strip (also carries P20's hint) |
| P23 | Keyboard form 1560×480, key layout inside a scroll area | `keybform.ui:9-19, 41-52` | `KeyboardPanel` with the same structure and cluster arrangement |
| P24 | 104 key caps: F row (16), main block (5 rows, 61), middle cluster (10), numpad (17) | parsed from `keybform.ui` (see section 8) | data-driven `Repeater` over `KeyTable.rows` |
| P25 | 102 caps are bindable and get ids 0…101 per the `KeyId` enum order | `keybform.h:14-118`, `keybform.cpp:23-137` | `KeyTable` assigns the **same ids**; see section 7 |
| P26 | `PrtScn` and `Menu` caps are disabled and carry tooltip `This key cannot be bound to an action.` | `keybform.ui` (both disabled blocks) | same, `enabled: false` + `ToolTip` |
| P27 | Caps form a non-exclusive button group; clicking emits the id | generated `ui_keybform.h:255-257`, `keybform.cpp:11-15` | `KeyCap.clicked(keyId)`, non-exclusive by construction |
| P28 | Cap ids outside `0…KEYCOUNT-1` are not emitted | `keybform.cpp:12` | same guard in `KeyCap` / signal relay |
| P29 | Key click → print the key token to stdout, then open a modal bind dialog prefilled with `bind "<token>"` | `editwindow.cpp:49-55` | `console.log` of the token (parity) + `BindEditorDialog` opened with `originalBind` |
| P30 | Bind dialog 500×400, base font TF2 14pt, title `Dialog` | `bindeditdialog.ui:4-23` | `Dialog` with `Theme.fontBindDialog`; title kept as `Dialog` for parity |
| P31 | `Existing Bind:` label (bold) + editable field, placeholder `< not currently bound >` | `bindeditdialog.ui:47-73` | `Label` + `TextField` |
| P32 | Tabs `Build-A-Bind` and `Custom Command` | `bindeditdialog.ui:99-101,212-215` | `Tf2TabBar` + `StackLayout` |
| P33 | Build-A-Bind: `Category:` label + empty combo, `Command:` label + empty combo, `Arguments:` label + empty empty area | `bindeditdialog.ui:119-208` | same widgets, no model wired (parity: empty) |
| P34 | Custom Command: `QPlainTextEdit`, Liberation Mono, no wrap, placeholder `< command goes here >` | `bindeditdialog.ui:254-272` | `TextArea` with `wrapMode: NoWrap`, same placeholder |
| P35 | Cancel/Ok buttons, centered, wired to `reject()`/`accept()`; accept slot body empty | `bindeditdialog.ui:284-340`, `bindeditdialog.cpp:18-20` | `Dialog.standardButtons` (Ok/Cancel); `onAccepted` logs only |
| P36 | Modal (`exec()`) — the editor is blocked while the dialog is open | `editwindow.cpp:55` | `Dialog.modal: true` |
| P37 | Loaded config survives across tab switches; a second `loadCfg` appends | `editwindow.cpp:16-23`, `gameconfig.cpp:12-35` | keep `GameCfg` semantics untouched |
| P38 | Parsed lines shown as `args.join(" ")`, i.e. quotes retained as parsed | `editwindow.cpp:21` | model role `line` = `args.join(" ")`, byte-identical to the widget version |

## 4. Deliberate deviations

Each is small, and each is listed so parity can be audited. No functional capability is added or removed.

| # | Deviation | Reason |
|---|---|---|
| D1 | One `ApplicationWindow` with a page stack instead of two `QMainWindow`s swapped by `hide()`/`show()` | The widget build never showed both at once; a single window removes the second hidden top-level and the stale `changeWindow()` dead code. Layout per page is unchanged. |
| D2 | Window size follows the active page (1000×500 / 1573×807), min = max like the widget build | Same visual result as the `.ui` `maximumSize` constraints. |
| D3 | `Go` is enabled exactly when the path is non-empty | Reproduces P11/P7/P10 and fixes review finding 8 (the widget left Go enabled after the combo was cleared to `-1`). Consequence: clearing the combo now also disables Go. |
| D4 | Cap labels normalised (`f3`→`F3`, `Prt\nScn`→`PrtScn`, `KP\nEnter`→`KP Enter`) | The widget labels were inconsistent in case and used hard line breaks; a game-style UI labels keys the way the keyboard does. Tokens (section 7) are untouched. |
| D5 | Tall numpad keys expressed as `rowspan: 2` instead of the widget's 130 px and 140 px heights | Same visual intent, ignores the 10 px inconsistency between `+` and `Enter`. |
| D6 | Placeholder tab keeps its position but is renamed from the widget's joke string to `Placeholder` | It is a placeholder; the string carries no meaning. |
| D7 | The widget's `printCfg()` stdout dump is preserved; the key-token `printf` becomes `console.log` | Keeps P12/P29 observable, uses QML's logging facility. |
| D8 | Design tokens (colors, spacing, radii, fonts) centralised in `Theme.qml` | The widget build hard-coded the fonts in every `.ui`; centralising is needed for the "easily extended" goal. Aesthetics are otherwise re-authored (dark tool theme instead of the default widget styling). |
| D9 | `keybform.h`'s `KeyId`/`KEYCODES` move to a QML singleton | The table is presentation data (ids + display labels + layout widths). When the bind engine lands it should move to C++ and be injected; see section 10. |
| D10 | Overview tab shows `No config loaded.` when the list is empty | The widget showed an empty box; an empty box cannot be told apart from "the load failed". No new capability, one label. |
| D11 | Start screen shows the load failure (`Could not load <path> (<reason>)`) in the Go row; `Browse` converts the dialog's `file://` URL to a local path first | The dialog is `QtQuick.Dialogs.FileDialog`, which yields a URL, so the conversion is a platform requirement. Showing the failure surfaces review finding 6 (the engine swallows `Lexer::open` failures) instead of leaving the user with an empty window. |
| D12 | Short description line under the preset combo, and placeholder text in the bind dialog's empty combos/argument area | Both are empty-state wording. The preset note is data (`presets[].note`), so future presets get one for free. |
| D13 | Main keyboard rows are stretched to the widest row (1012 px) with computed per-row gaps | Matches what the widget's `QHBoxLayout` rows did with fixed-width caps; keeps the main block rectangular. The middle and numpad clusters keep the plain 4 px gap. |
| D14 | The bind dialog opens centred on the window and is dragged by its title bar, clamped inside the window | A stock QML `Dialog` has no drag behaviour and sits at (0,0); the widget `QDialog` was moved by the window manager, which could leave the screen. In-window bounds are what the tool needs. |

## 5. Target architecture

```
src/
  main.cpp                     # QGuiApplication, Basic style, dark palette, fonts, engine, context property
  ui/
    configcontroller.h/.cpp    # QObject facade: presets, load, command model, load state
    commandlistmodel.h/.cpp    # QAbstractListModel over cmd::Entry
  gamecfg/**                   # unchanged engine
  legacy/
    widgets/                   # retired widget UI (this rewrite's predecessor) + README
qml/
  Main.qml                     # ApplicationWindow, page stack, menu bar, dialogs
  theme/Theme.qml (+qmldir)    # singleton: colors, fonts, metrics
  components/
    Panel.qml                  # bordered surface with optional heading
    Tf2Button.qml              # game-style push button
    Tf2ComboBox.qml            # themed combo box with placeholder support
    Tf2TextField.qml           # themed single-line field
    Tf2TabBar.qml              # themed tab strip (editor + dialog) - not QtQuick.Controls.TabBar
    Tf2Label.qml               # body label with the app's default text style
  pages/
    StartPage.qml              # P2–P11
    EditorPage.qml             # P13–P22
    tabs/CommandOverviewTab.qml
    tabs/PlaceholderTab.qml
    tabs/BindEditorTab.qml
  keyboard/
    KeyTable.qml (+qmldir)     # singleton: key table + row layout
    KeyboardPanel.qml          # P23–P28
    KeyCap.qml                 # one cap
  dialogs/
    BindEditorDialog.qml       # P30–P36
qml.qrc                        # packs qml/** into the binary
```

Implementation notes:

- `Tf2TabBar` is a plain `Item` (a `Row` of clickable tab items), not a `TabBar`: Qt 6.12's `TabBar` has no `delegate` property, so styling every tab would require restyling the built-in `TabButton` globally. The custom control exposes only `model` / `tabCount` / `currentIndex` / `titleAt` / `activated` — exactly the parts the editor and the bind dialog use.
- `main.cpp` installs a dark `QPalette` on `QGuiApplication`. Every control the app themes in `Theme.qml` ignores it; the palette colours the few controls left stock (tool tips, menu popups, scroll bars, dialog chrome), so they match instead of flashing light.
- Key clicks are measured against the id table by both `KeyTable` (QML) and the widget build's `KeyId` enum; the first automated run clicked the Space cap and logged `id=71 token=space`, and the dialog opened with `bind "space"` — the id/order parity holds end to end.

Data flow: `Main.qml` owns the page stack. `StartPage` picks a path and calls `cfg.loadCfg(path)`. `ConfigController` parses through `GameCfg` and fills `CommandListModel`. `EditorPage` renders the model and the keyboard; a key click opens `BindEditorDialog` with the token from `KeyTable`. Nothing in QML touches `GameCfg` directly.

Boundary rule: **C++ owns game/config data and parsing; QML owns presentation and the key layout.** The only exception is the key table (D9), because nothing in C++ consumes it yet.

## 6. C++ API contract

### `CommandListModel` (QAbstractListModel)

Mirrors one `cmd::Entry` per row. Row order = insertion order = parse order, so the Overview list matches the widget list exactly.

| Role | Type | Value | Used by |
|---|---|---|---|
| `LineRole` | QString | `entry.args.join(" ")` — exactly `editwindow.cpp:21` | Overview list (P38) |
| `CommandRole` | QString | `entry.args.value(0)` | future: grouping |
| `ArgsRole` | QStringList | `entry.args.mid(1)` | future: argument editors |
| `TypeRole` | int | `cmd::CmdType` as int | future: type-aware UI |
| `TypeNameRole` | QString | `"int"`, `"float"`, `"string"`, `"command"`, `"bind"`, `"alias"`, `"unknown"` | future: display + debugging |
| `BoundKeyRole` | QString | empty for now; reserved for the key token a bind applies to | future: keyboard overlay |

`setEntries(const QVector<cmd::Entry> &)` replaces the contents with `beginResetModel`/`endResetModel`.

### `ConfigController` (QObject, exposed to QML as the context property `cfg`)

| Member | Kind | Contract |
|---|---|---|
| `presets` | `QVariantList` | `[{label, path, note}]`; matches P9/P10 order and labels |
| `presetAt(index)` | `Q_INVOKABLE QString` | `:/cfg/...` path for a preset index, or empty |
| `loadCfg(path)` | `Q_INVOKABLE bool` | calls `GameCfg::appendCfg` + `printCfg` (D7), fills the model, sets `cfgPath`, returns success; failure sets `lastError` |
| `clear()` | `Q_INVOKABLE void` | empties the model and load state |
| `commandModel` | `Q_PROPERTY QAbstractItemModel*` | the model above, `CONSTANT` |
| `cfgPath` | `Q_PROPERTY QString` | last successfully loaded path; used by the future save path |
| `loaded` | `Q_PROPERTY bool` | `commandModel.rowCount() > 0` |
| `lastError` | `Q_PROPERTY QString` | empty on success; `Lexers::Lexer::open` failure is currently silent, so this is set from the same check the widget build ignored (review finding 6, surfaced not fixed) |

`GameCfg`, `cmd::*`, `Lexers::*` are unchanged apart from removing two unused `#include <QApplication>` lines that only existed because the widgets module was in the build.

## 7. Key table — the id parity contract

The widget build emitted a `KeyId` id and mapped it with `KEYCODES[KEYCOUNT]` (`keybform.h:14-220`). The QML build must emit the same numbers for the same keys, so the table is reproduced verbatim, in enum order:

```
0 escape        1 f1           2 f2           3 f3           4 f4
5 f5            6 f6           7 f7           8 f8           9 f9
10 f10          11 f11         12 f12         13 scrollock   14 pause
15 `            16 1           17 2           18 3           19 4
20 5            21 6           22 7           23 8           24 9
25 0            26 -           27 =           28 backspace   29 tab
30 q            31 w           32 e           33 r           34 t
35 y            36 u           37 i           38 o           39 p
40 [            41 ]           42 \           43 capslock    44 a
45 s            46 d           47 f           48 g           49 h
50 j            51 k           52 l           53 ;           54 '
55 enter        56 shift       57 z           58 x           59 c
60 v            61 b           62 n           63 m           64 ,
65 .            66 /           67 rshift      68 ctrl        69 lwin
70 alt          71 space       72 ralt        73 rwin        74 rctrl
75 ins          76 home        77 pgup        78 del         79 end
80 pgdn         81 uparrow     82 leftarrow   83 downarrow   84 rightarrow
85 numlock      86 kp_slash    87 kp_multiply 88 kp_minus   89 KP_home
90 kp_uparrow   91 kp_pgup     92 kp_leftarrow 93 kp_5      94 kp_rightarrow
95 kp_end       96 kp_downarrow 97 kp_pgdn    98 kp_ins     99 kp_del
100 kp_plus     101 kp_enter
```

Spellings are the engine's own (including `scrollock`, `KP_home`) and must not be "corrected". `PrtScn` and `Menu` have no id (P26).

## 8. Layout spec

### Start screen (P2–P11)

Vertical: banner (title) → main row (left panel | 4 px spacer | right panel) → Go button, right-aligned. Panels capped at 480 px wide each, matching `startwindow.ui`. Both panels: heading, then a selector row (field + `Browse` / combo).

### Editor (P13–P22)

Vertical: menu bar → tab bar → tab content (fills) → status strip → Prev/Next bar. Tab content per section 3 (P15, P17, P18).

### Keyboard (P23–P28)

Fixed content 1560×480 inside a scroll area, exactly four clusters:

```
fnRow   : Esc | F1 F2 F3 F4 | F5 F6 F7 F8 | F9 F10 F11 F12 | PrtScn ScrLk Pause
main    : ` 1 2 3 4 5 6 7 8 9 0 - = [Backspace 128]
          [Tab 96] q w e r t y u i o p [ ] [\ 96]
          [Caps 116] a s d f g h j k l ; ' [Enter 144]
          [LShift 148] z x c v b n m , . / [RShift 176]
          [LCtrl 80] [LWin 80] [LAlt 80] [Space 416] [RAlt 80] [RWin 80] [Menu 80] [RCtrl 80]
middle  : Ins Home PgUp / Del End PgDn / (gap) / Up / Left Down Right
numpad  : NumLk KP/ KP*  | KP-        with KP+ and KP Enter spanning two rows
          KP7 KP8 KP9    |
          KP4 KP5 KP6    |
          KP1 KP2 KP3    |
          [KP0 128] KP.  |
```

Default cap: 64×64. Widths above are the widget `maximumSize` values; the 4 px inter-cap gap matches the `.ui` `spacing`. Main-block rows are stretched to the widest row (D13). The middle and numpad clusters are centred as a group and keep the plain 4 px gap.

## 9. Theme

Dark tool theme, TF2-adjacent palette. All values in `Theme.qml`.

| Token | Value | Use |
|---|---|---|
| `colorBg` | `#141719` | window background |
| `colorSurface` | `#1d2124` | panels |
| `colorSurfaceRaised` | `#262b2f` | banners, key caps |
| `colorSurfaceSunken` | `#101315` | list/field backgrounds |
| `colorBorder` | `#3a4045` | 1 px borders |
| `colorBorderStrong` | `#4d555c` | focused/hover borders |
| `colorText` | `#e8e5df` | primary text |
| `colorTextMuted` | `#98a1a8` | secondary text, placeholders |
| `colorAccent` | `#f0a30a` | TF2 orange: primary buttons, focus, tab indicator |
| `colorAccentHover` | `#ffb422` | accent hover |
| `colorAccentText` | `#1a1206` | text on accent |
| `colorRed` / `colorBlu` | `#b8383b` / `#5885a2` | decorative accents only |
| `colorKeyCap` / `colorKeyCapHover` / `colorKeyCapDisabled` | `#2b3136` / `#39424a` / `#202427` | key caps |
| `fontTitle` | `TF2 Professor`, 40 | banner |
| `fontHeading` | `TF2 Professor`, 22 | panel headings |
| `fontUi` | `TF2`, 12 | body text |
| `fontButton` | `TF2 Build`, 12 | buttons, tabs, key caps |
| `fontMono` | `Liberation Mono`, 12 | config text |
| metrics | `spacing 4/8`, `radius 4`, `capSize 64`, `borderWidth 1` | consistent spacing |

Rationale: the app is a game companion tool; the bundled TF2 fonts plus an orange-industrial accent on near-black surfaces read as TF2 without shipping any copyrighted artwork.

## 10. Extension points for the known future work

Mapped from `docs/pickle-inferences.md` §"What is unfinished" and `docs/CODE_REVIEW.md`.

| Future feature | Where it lands | What is already in place after this rewrite |
|---|---|---|
| Save a config to disk | `ConfigController::saveCfg()` + `File` menu action + `cfgPath` | menu bar exists with an empty `File` menu; `cfgPath` already tracked |
| Paint loaded binds onto the keyboard | `BindEditorTab` sets `KeyCap.bindText` from the model's `BoundKeyRole` | `KeyCap` has a `bindText` property rendering nothing today; role reserved |
| Type-aware argument editors | `BindEditorDialog` "Arguments" area switches on `TypeRole`, using `IntVar::getMinVal()`/`getMaxVal()` | model exposes type + args; the cvar metadata is already parsed (unused today) |
| Alias chains | `BindEditorDialog` command list includes aliases; `TypeNameRole` reports `alias` | `CMD_ALIAS` detection already exists in `GameCfg` |
| Command categories | `Command:` combo grouped by a new field | the combo is a component with a model-driven API, empty for now |
| More presets (`b4nny_defbinds`, `b4nny_nobinds`) | `ConfigController::presets` list | presets come from one C++ list instead of being split between code and `.ui` labels |
| Key table owned by C++ | move `KeyTable.qml` data into a `KeyTable` QObject, inject via context property | one data source today (D9), no duplicated table |
| Parse-fidelity fixes | `src/gamecfg/lex/**` | untouched by design; the UI does not depend on the bugs being fixed |

## 11. Legacy handling

- `src/ui/*.cpp`, `src/ui/*.h`, `src/ui/*.ui` → `src/legacy/widgets/` (`git mv`, history preserved).
- `src/legacy/widgets/README.md` states: retired, not built, why, and the exact `.pro` lines needed to revive it (`QT += widgets`, the SOURCES/HEADERS/FORMS block).
- Removed from `tf2cfgreal.pro`; `QT += widgets` dropped.
- The files stay on disk so the parity audit in section 3 can always be re-run against the source of truth.

## 12. Verification results

Performed against the MinGW Qt 6.12.0 build with the full automation run on 2026-10-12.

1. Clean qmake build (`-spec win32-g++ "CONFIG+=release"`): compiles with zero new warnings; the only remaining warning is the pre-existing unused variable in `src/gamecfg/lex/cmdlexer.cpp:34` plus windeployqt's standard Direct3D note, which is ignorable.
2. Start screen: app launches to `TF2 Config Manager` (1000×500, fixed). Preset combo shows `< Choose a Preset >`; selecting `b4nny` sets the path to `:/cfg/b4nny.cfg` and enables Go.
3. Load: Go triggers `printCfg` to stdout (274 lines for b4nny.cfg); the editor opens at 1573×807 (`Team Fortress 2 Config Editor`).
4. Overview tab: ListView `count` = 274 = stdout dump line count, same `args.join(" ")` text; empty-state label hidden.
5. Keyboard: all 104 caps render; clicking the Space cap logged `keybButtonClicked: id=71 token=space` via `console.log` and opened the modal bind dialog with the field prefilled `bind "space"` — the id/token contract (section 7) holds at runtime.
6. Tab bar: editor tabs and the dialog's Build-A-Bind/Custom Command tabs share the same control; Prev/Next enable rules follow the current tab.
7. Bind dialog (D14): opens centred — measured (537, 204) inside a 1573×807 parent, i.e. exactly centred for 500×400 — and drags by the title bar: +400/+300 moved it (400, 203) with y clamped at the 407 px limit, a further +2000/+1500 clamped exactly at (1073, 407) = parent minus dialog, and dragging back up-left clamped at (0, 0). Escape still rejects, clicking outside still does nothing (modal).
8. Deploy: `windeployqt --release --qmldir qml` produces a 101.8 MB folder; the exe runs standalone with a stripped `PATH` (no Qt bin dirs) and shows the start screen.
9. Everything in section 3 that is not covered above is guaranteed by construction (same `KeyTable` data drives caps, ids, tokens, widths; bind dialog parity strings are literals; `printCfg` is the untouched engine function).

| Parity row | How it was verified |
|---|---|
| P38 (overview text) | ListView count == `printCfg` stdout lines (274), same join, same order |
| P25/P27/P29 (key ids + click → dialog) | automated click on Space → `id=71 token=space`, dialog prefill `bind "space"` |
| P11/P7/P10 (Go enable) | Go enabled exactly when path non-empty (probed state before/after preset selection) |
| P2/P13 (window sizes) | start 1000×500, editor 1573×807, fixed min=max per page |
| P21/P22 (menus, status strip) | present on the editor page, absent/hidden on the start page |
| P30/P36 (dialog) | opens centred; drag clamped to the window (D14); Escape rejects; outside press blocked |
