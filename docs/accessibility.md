# Accessibility

Date: 2026-10-12
Scope: the QML layer (`qml/**`, 17 files) — the C++ engine exposes no user-facing controls
of its own, and `ConfigController` is data only.
Method: every file read line by line, `Accessible.*`/`Keys`/`focus` usage grepped, and
contrast ratios computed for every foreground/background pair actually used, with the WCAG
2.x relative-luminance formula.

This is a desktop Qt application, not a web page, so WCAG does not formally apply. It is
still the shortest way to talk about what is missing, and the platform equivalents
(Windows UI Automation via `Accessible.*`, focus rings, tab order) map onto the same
success criteria: 1.4.3/1.4.11 contrast, 2.1.1 keyboard, 2.4.3 focus order, 2.4.7 focus
visible, 4.1.2 name/role/value.

**Bottom line: the app is usable with a keyboard up to the point of touching a key.** Every
screen can be driven with Tab and Enter until the keyboard panel appears, and then it stops:
the 104 key caps and the three tabs are `MouseArea`-only items with no focus and no role, so
a keyboard-only or screen-reader user cannot bind a single key. Nothing in the tree carries
an `Accessible.*` property. Contrast is good for body text and fails at nine pairs, three of
which are user-visible text or control boundaries.

---

## 1. Inventory

| Element | Built as | Keyboard | Focus visible | Accessible name/role |
|---|---|---|---|---|
| Browse, Go, Prev, Next buttons | `Tf2Button` (Qt Quick Controls `Button`) | yes (Controls default) | **no** — no `activeFocus` anywhere in `Tf2Button.qml` | from `text` (Qt default) |
| Config path field | `Tf2TextField`, `readOnly` | yes | yes — accent border (`Tf2TextField.qml:25`) | placeholder only, no label |
| Preset combo | `Tf2ComboBox` | yes | yes — border, but at 2.48:1 | from `textRole` |
| Editor tabs (3) | `Tf2TabBar`, custom `Item` + `MouseArea` | **no** (`Tf2TabBar.qml:82`) | n/a | **none** |
| Key caps (104) | `KeyCap`, `Rectangle` + `MouseArea` (`KeyCap.qml:110`) | **no** | n/a | **none** |
| Keyboard scrolling | `Flickable` + `WheelHandler` (`KeyboardPanel.qml:125`) | **no** — wheel only | n/a | n/a |
| Keyboard zoom | `Shortcut` ×3 (`KeyboardPanel.qml:341-352`) | yes — `Ctrl`+`=`/`-`/`0` | n/a | n/a |
| Bound-key tooltip | `ToolTip` on hover (`KeyCap.qml:130`) | **no** — hover only | n/a | **none** |
| Status hint (prev/next names) | hover-driven (`EditorPage.qml:94,102`) | **no** — hover only | n/a | n/a |
| Bind dialog controls | stock Controls (`ComboBox`, `CheckBox`, `RadioButton`, `TextArea`, Ok/Cancel) | yes | style/palette driven | from text |
| Dialog suggestion list | stock popup + `Keys` up/down/return (`BindEditorDialog.qml:494-500`) | yes | yes | from text |
| Menu bar | `File` with `Save`/`Save As.` actions, `Edit`/`View`/`Help` empty (`Main.qml`) | yes | yes | actions announce text + Ctrl+S / Ctrl+Shift+S; the three empty menus still announce as empty - parity P21 |

Stock controls inherit a dark palette (`src/main.cpp:16-47`), so the un-themed ones
(CheckBox, RadioButton, ToolTip, menu popups) are legible; they simply do not match the rest
of the UI.

## 2. Findings

### A1 — The key caps cannot be reached without a mouse (high)

`KeyCap` is a `Rectangle` with a `MouseArea`. It has no `activeFocusOnTab`, no `Keys`
handler, and no `Accessible.role`. Binding a key is the app's one core action, and it is
unreachable by keyboard and invisible to a screen reader. Nothing else in the app is this
bad: every other control is at worst unlabelled.

### A2 — The editor tabs are mouse-only (medium)

`Tf2TabBar` is a custom control chosen for its look (deviation in section 4 of
`docs/legacy/ui-qml-rewrite.md`). It is `Item` + `MouseArea`: no focus, no arrow-key
navigation, no role. The Prev/Next buttons (`EditorPage.qml:90-104`) remain a working
keyboard path to every tab, so this is an inconvenience rather than a dead end — but a user
who tabs into the editor lands on Prev, not on the tabs.

### A3 — No focus indicator on buttons (medium)

`Tf2Button.qml` never reads `activeFocus`, so a keyboard user gets no visual cue at all.
`Tf2TextField` (accent border, 8.91:1) and `Tf2ComboBox` do show focus — but the combo's
indicator is `colorBorderStrong` on `colorSurfaceSunken` at **2.48:1**, below the 3:1 that
WCAG 1.4.11 asks of a control boundary. It is the only cue, so it has to clear 3:1.

### A4 — No `Accessible.*` anywhere (high for screen readers, zero cost elsewhere)

`grep -r "Accessible\." qml/` returns nothing across all 17 files. Qt Quick Controls
supplies role and name for the standard controls, so those are mostly covered by accident.
Everything custom is not:

| Needs a name/role | Where |
|---|---|
| key cap: role, its key label, its bind command | `KeyCap.qml` |
| tab: role, title, selected state | `Tf2TabBar.qml` delegate |
| config path field: a label for the placeholder | `StartPage.qml:109` |
| existing-bind field: the key it belongs to | `BindEditorDialog.qml:370` |
| argument editors: what each one edits | `BindEditorDialog.qml` argument area |

### A5 — Panel scrolling is wheel-only (medium)

`Flickable` does not scroll on arrow keys. With the keyboard now opening at 1000×500
(deviation D2) the panel is scrolled more often than it used to be, and `KP Enter` and the
outer caps still sit outside the viewport. There is a mouse-wheel affordance and nothing
else.

### A6 — Hover-only information (low)

The bound-key tooltip and the prev/next status hint exist only on hover. A keyboard user can
reach the same facts (the bind dialog shows the command when opened; the tab titles are in
the tab strip), so nothing is strictly unreachable — but the tooltip itself should appear on
focus once caps can be focused.

### A7 — Nine contrast pairs below their threshold

Computed for every pair in use; 16 of 25 pass, several with large margins (`colorText` on
`colorSurface` is 12.90:1). The failures:

| Foreground | Background | Ratio | Needs | What it is | Nearest fix |
|---|---|---|---|---|---|
| `colorTextDisabled` | `colorSurface` | 2.82 | 4.5 | disabled label in a Panel | **accepted as decoration** (2026-10-13) |
| `colorTextDisabled` | `colorBg` | 3.13 | 4.5 | disabled button label | **accepted as decoration** (2026-10-13) |
| `colorRed` | `colorSurface` | ~~2.83~~ 5.45 | 4.5 | error text in a Panel | fixed → `#da7a7d` |
| `colorRed` | `colorBg` | ~~3.14~~ 6.05 | 4.5 | load error text | fixed → `#da7a7d` |
| `colorBorderStrong` | `colorSurfaceSunken` | ~~2.48~~ 3.03 | 3.0 | field border **and the combo focus ring** | fixed → `#59626a` |
| `colorKeyCapBoundBorder` | `colorKeyCapBound` | ~~2.26~~ 3.01 | 3.0 | border of a bound key | fixed → `#b76b2d` |
| `colorKeyCapBorder` | `colorKeyCap` | ~~1.51~~ 3.12 | 3.0 | cap boundary | fixed → `#707d88` |
| `colorBorder` | `colorSurface` | ~~1.54~~ 3.02 | 3.0 | panel border | fixed → `#626c74` |
| cap label on `colorKeyCapDisabled` | | ~~4.48~~ ~5.4 | 4.5 | `PrtScn` / `Menu` cap label | fixed: label token → `colorKeyCapText`, cap opacity 0.55 → 0.65 |

Post-fix note (2026-10-13): the audit's 4.48 for the disabled cap label measured
`colorKeyCapText`, but the code used `colorTextDisabled`, which cannot reach 4.5 on
`colorKeyCapDisabled` at any opacity (max 2.73). The fix therefore gives the label the
light token it was assumed to have, and the whole-cap opacity is 0.65.

`colorRed` sits on three backgrounds in real use (bg, surface, and the toast's raised
surface); the doc's original two fix values only clear 4.5 on one or two of them, so the
token went to `#da7a7d`, which passes on all three.

The live guard is `tools/contrast_check.py` (W8): re-run after any `Theme.qml` change.

Two caveats, so these numbers are not read as gospel:

- WCAG 1.4.3 exempts *inactive* controls, so `colorTextDisabled` is arguably compliant at
  3.13. It is still hard to read, and the "nearest fix" values are computed, not designed.
- The cap and panel borders are decorative in practice — the caps are identified by their
  labels and laid out in a grid — so 3:1 is the strict reading, not the only one.

The two that are not arguable are `colorRed` (that is error text a user must read) and
`colorBorderStrong` (that is the only focus ring on a combo box).

### A8 - The menu bar announces empty menus (low, mostly resolved)

Parity P21: `File`, `Edit`, `View`, `Help` existed with no actions. The `File` menu now
carries `Save` and `Save As.` (deviation D20), so three menus remain empty and a screen
reader announces those as empty. Finishing the job - removing the three stubs or giving
them content - is a deviation decision, not a bug fix.

## 3. What the fixes entail

Ordered by value per line. Estimates are for the change itself, not its verification.

| # | Work | Size | Risk | Notes |
|---|---|---|---|---|
| W1 | `Tf2Button` focus ring | DONE 2026-10-13: `activeFocus` case in the background - 2 px accent border. | ~4 lines, fixes A3 for every button at once. Do first. |
| W2 | `KeyCap`: `activeFocusOnTab` on bindable caps, `Keys.onReturnPressed`/`onSpacePressed` → `activated(keyId)`, focus ring in the existing `Rectangle` | DONE 2026-10-13: tab-reachable bindable caps, Return/Space activate, focus shows the hover highlight, tooltip on focus (W7 folded in). | low | Must not fire for the two non-bindable caps - they stay out of the tab chain. |
| W3 | `Tf2TabBar`: `activeFocusOnTab`, `Keys.onLeftPressed`/`onRightPressed` move + activate, focus ring on the delegate | DONE 2026-10-13: strip is tab-reachable, Left/Right wrap and activate, focused tab shows the 2 px accent ring, `Accessible.PageTab` on each tab (W4's tab part). | low | Alt+1..3 is a cheaper first cut if arrow semantics are not wanted. |
| W4 | `Accessible.role`/`name`/`description` on `KeyCap`, the tab delegate, and labels for the two read-only fields | DONE 2026-10-13 for `KeyCap` (button role, label as name, bind line as description) and the two read-only fields (config path, existing bind). Tab delegate remains (W3's scope). | none | Purely additive; no visual change. |
| W5 | Contrast: `colorRed` → `#da7a7d`, `colorBorderStrong` → `#59626a` | DONE 2026-10-13: `colorRed` #da7a7d (passes on all three error surfaces), `colorBorderStrong` #59626a, cap/panel borders fixed (#707d88 / #b76b2d / #626c74), disabled-cap label → `colorKeyCapText` @ 0.65 opacity. `colorTextDisabled` accepted as decoration. | visual | The two non-negotiable values change the palette; the rest are design calls. |
| W6 | Keyboard scrolling: `KeyNavigation.left/right/up/down` on the panel, or a `Keys` handler driving `flick.contentX/Y` | DONE 2026-10-13: `Keys.onPressed` on the panel drives `flick.contentX/Y` (clamped, 60 px per press). | low | Prefer `KeyNavigation` - it handles the boundary for free. |
| W7 | `KeyCap` tooltip: `visible: hovered \|\| focusWithin` once W2 lands | DONE 2026-10-13 (with W2): tooltip shows on focus too. | none | - |
| W8 | Move the contrast script into `tools/contrast_check.py` and run it after any palette change | DONE 2026-10-13: `tools/contrast_check.py` (20 checked pairs + 2 accepted-decorational, exits nonzero on failure). | none | A one-off script, but this one guards a live constraint rather than a finished dataset, which is why it survives the `tools/` cleanup. |

**Skipped deliberately:** a roving arrow-key grid over the 104 caps (W2 plus ~40 more
lines, and arrow semantics across four differently-shaped clusters is a real design
question, not a checkbox); `Accessible.ignored` annotations for decorative items (no noise
today to suppress); a high-contrast theme mode (a palette redesign, not a fix).

Not an accessibility item, but noted here because it was measured alongside: `qmllint`
reported 41 `ComponentBehavior: Bound` advisories across the QML tree. Seven files now carry
the pragma, which cleared 18 of them; the remaining 23 are context-property reads and
`KeyboardPanel` delegates reaching an enclosing component, and need a structural change
rather than a pragma.

## 4. Verification

Nothing above is covered by `tests/tst_engine.cpp`, which tests C++ only. QML would need
`QtQuick.Test`, which is a new dependency and a new build target — the same decision CI
(#22) will force anyway.

Until then, the manual pass, which is short:

1. Launch, press Tab repeatedly: every control on the start screen takes focus, shows a
   visible ring, and Enter/Space activates it.
2. Load a config, go to the editor tabs: Left/Right changes tab; the ring is visible.
3. On the keyboard panel: Tab reaches a cap, the ring is visible, Enter opens the bind
   dialog for that key; arrows scroll the panel to the outer keys.
4. Open the bind dialog, Tab through it: every combo, checkbox, radio, field and the Ok/Cancel
   pair takes focus and shows a ring.
5. Contrast: re-run the ratio check after any `Theme.qml` change.

Not verifiable here: real screen-reader output. No assistive technology is installed on this
machine, so every `Accessible.*` claim above is a claim about what Qt is told, not about
what a reader announces. Anyone with NVDA, Narrator or Orca should re-run step 4 and report
what they hear before this document claims anything more.