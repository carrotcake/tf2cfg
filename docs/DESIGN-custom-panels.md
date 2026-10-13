# Design: Custom Command Panels (Build-A-Bind)

Status: design, pending review. Feature: `voicemenu` gets its own authoring panel;
the mechanism generalizes to future commands with particular argument semantics.

## 1. Problem

`voicemenu <menu> <item>` is not a generic numeric-argument command. It drives the
in-game quick-chat system:

- `voicemenu N` (one argument) **opens menu N** on the in-game GUI; the player then
  picks a line.
- `voicemenu N M` (both arguments) makes the character **say the line immediately**,
  with no UI prompt.
- Each (menu, item) pair is a named voice line ("MEDIC!", "Nice Shot!", ...). The
  generic argument editors turn these into two meaningless number boxes.

More commands may need this kind of tailored authoring later, so the panel must be a
mechanism, not a one-off.

## 2. Decisions (user-confirmed 2026-10-13)

| # | Decision | Choice |
|---|----------|--------|
| D-a | Placement | Custom panel **replaces the generic argument editors** inside Build-A-Bind when the selected command has one. The command combo stays visible. |
| D-b | Line labels | **Generic labels** (one static table). Class-specific variants are not modeled — the bound value is identical for every class. |
| D-c | Per-menu item counts | Menus 0/1 have 9 lines (0..8), menu 2 has 8 (0..7). The panel's data table encodes counts; the catalog bound (`voicemenu` item ≤ 8) stays as the global safety net. |
| D-d | Open-menu vs say-line | **Single implicit flow**: picking a menu alone binds `voicemenu N`; picking a line in the grid binds `voicemenu N M`. No separate mode switch. |
| D-e | Source of truth | The panel writes its result into the existing **Custom Command field** (what OK applies). Prefill reads `voicemenu N M` back into the panel on open. OK flow unchanged. |

## 3. Generalizable panel contract

A custom panel is a QML component that fulfils this contract (documented here and in
the panel file's header):

    // Custom panel contract (docs/DESIGN-custom-panels.md §3):
    //   property var initialArgs  - args of the binding being edited, parsed;
    //                               [] when the key carries no binding
    //   signal authored(string text) - the panel emits its authored command; the
    //                               dialog writes it to the Custom Command field
    //   function prefill(parts)   - called when the key's binding parses to this
    //                               command; parts[0] is the command name itself

The dialog owns everything else:

- **Registry**: `property var customPanels` in `BindEditorDialog` mapping the
  normalized command name (`voicemenu`) to a `Component`. Adding a future panel is:
  new QML file in `qml/dialogs/panels/`, one qrc entry, one registry line. No engine
  or model changes.
- **Activation**: when `commandInfo.name` (lowercased) is a registry key, the Loader
  instantiates the panel, sets `initialArgs`, and the generic argument area
  (arg editors + incrementvar step field) is hidden. The bind/bindtoggle mode toggle,
  the command combo and the Custom Command field stay as they are.
- **Wiring**: the dialog connects the panel's `authored(text)` to
  `customCommand.text = text` and skips the generic `regen()` while a panel is active,
  so the generic generator can never stomp the panel's output.
- **Prefill routing**: `prefillFromCommand()` hands the parsed parts to
  `panel.prefill(parts)` instead of the generic arg-model fill when a panel is active.
- **Fallback safety**: if the bound text parses to `voicemenu` but the args are not
  integers the panel understands, `prefill` leaves the panel on its default state and
  the Custom Command field still shows the raw text (nothing is lost or rewritten).

## 4. VoiceMenuPanel

New file `qml/dialogs/panels/VoiceMenuPanel.qml` (registered in `qml.qrc`).

### Data (generic labels, per-menu counts, D-b/D-c)

    property var menus: [
        { key: "Z", title: "Voice menu",   lines: ["Medic!", "Thanks!", "Go go go!", "Move up!", "Go left!", "Go right!", "Yes!", "No!", "Pass to me!"] },
        { key: "X", title: "Alerts",       lines: ["Incoming!", "Spy!", "Sentry ahead!", "Teleporter here!", "Dispenser here!", "Sentry here!", "Activate charge!", "ÜeberCharge ready!", "Pass to me!"] },
        { key: "C", title: "Reactions",    lines: ["Help!", "Battle cry!", "Cheers!", "Jeers!", "Positive!", "Negative!", "Nice shot!", "Good job!"] },
    ]

Line names follow the wiki's community-maintained mapping (`voicemenu 0 0` = "Medic!"
through `voicemenu 2 7` = "Good job!"; item 8, "Pass to me!", exists on menus 0/1
only). Class-specific variants are intentionally not modeled.

### UI

- **Menu selector**: three segmented buttons (Z / X / C with their titles). Selecting
  one sets the panel's menu index.
- **"Open this menu" button**: emits `authored("voicemenu " + menuIndex)`. This is the
  one-argument form — the key opens the menu in-game.
- **Line grid**: a button per line of the selected menu (Repeater over
  `menus[menuIndex].lines`). Clicking a line emits `authored("voicemenu " + menuIndex
  + " " + itemIndex)`. The clicked cell stays highlighted as the current selection.
- **State**: `menuIndex` and `itemIndex` (-1 = "open menu only"). Any change emits the
  corresponding `authored(...)`; the dialog mirrors it into Custom Command.
- **prefill(parts)**: expects `["voicemenu", N]` or `["voicemenu", N, M]` with integer
  N in 0..2 and M within the menu's line count; selects menu/item accordingly. Any
  other shape (non-integer, out of range, extra args) → keep defaults and emit nothing,
  so a foreign `voicemenu ...` variant is never silently rewritten.

### Styling

Theme tokens only (`Theme.*`), same components as the rest of the dialog
(`Tf2Button` etc.). The selected line cell uses the dialog's existing highlight color
pairing; no new colors.

## 5. Tests

The engine test suite cannot drive QML. Coverage that exists stays green:

- The catalog row `voicemenu,0,int,0,2` / `voicemenu,1,int,0,8` remains the numeric
  safety net; `argBoundsAttachFromCompanionFile` already exercises the loader path.
- The panel is verified manually per AGENTS.md: open the dialog, select `voicemenu`,
  check the three menus render with the right line counts (9/9/8), "open menu" writes
  `voicemenu 0`, a line click writes `voicemenu 1 4`, and a key pre-bound to
  `voicemenu 1 4` re-opens with that line highlighted.

## 6. Rollout

1. Design doc (this file).
2. `VoiceMenuPanel.qml` + qrc entry + registry + activation/prefill routing in
   `BindEditorDialog.qml` — one commit, it is inert without the registry entry.
3. Manual verification by the user; adjust per feedback.

## 7. Out of scope

- Per-class line labels (D-b).
- Autogenerated panels from catalog metadata — panels are hand-written by design; the
  catalog's `argsSpec` continues to serve the generic editors.
- Other candidate commands (none identified yet; the registry makes them cheap).
