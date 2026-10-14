# Roadmap - tf2cfg

The single list of open work. Fixed, implemented or deliberately dismissed items do not
appear here; their records live in `docs/legacy/` (audit, code review, migration notes)
and stay there.

Order within each section is priority order. Open code findings from the 2026-10-13
audit (`docs/AUDIT.md`) live in section 6; dismissed items are not listed here at all.

## 1. CI workflow (the 2026-10-12 audit's only open finding, legacy #22)

DONE 2026-10-13: `.github/workflows/ci.yml` — Linux only (`ubuntu-latest`), on push to
`qml-rewrite`/`master` and on PRs. Jobs: install Qt 6 apt packages, build the app,
build + run `tf2cfg_test` (offscreen platform), gate `qmllint` at 0 errors (the 28
known warnings tolerated), run `tools/contrast_check.py`. The app binary is only
compiled, never launched (no display); the test binary runs headless via
`QT_QPA_PLATFORM=offscreen`. First Linux compile of this codebase — the CI run is the
verification.

DONE 2026-10-14, same workflow: AppImage packaging. `linuxdeploy` + `linuxdeploy-plugin-qt`
bundle the built binary with Qt 6.4.2 libs/plugins (QML modules come from the
`qml6-module-*` runtime packages); the icon is rendered at build time with ImageMagick
from the repo's TF2 font (`res/icon/tf2cfg.svg` is the mockup, `tf2cfg.desktop` the
menu entry). The AppImage is smoke-tested headless (offscreen run for 15 s, exit 124 =
success), uploaded as an artifact on every run, and attached to a GitHub Release when a
`v*` tag is pushed (`softprops/action-gh-release@v3`). Compatibility note: the AppImage
targets current distros (glibc 2.39 / Qt 6.4.2 floor) — accepted, per project decision.

## 2. Accessibility (`docs/accessibility.md` - evaluated, sized, deliberately deferred)

The gap is real: the app is keyboard-usable up to the keyboard panel, then stops - the
104 key caps and the three editor tabs are `MouseArea`-only, and nothing carries
`Accessible.*`. `docs/accessibility.md` holds the findings (A1-A8) and the sized work
items (W1-W8); the priorities:

| # | Work | Notes |
|---|------|-------|
| W1 | `Tf2Button` focus ring | DONE 2026-10-13: 2 px accent border on `activeFocus`. |
| W2 | `KeyCap` keyboard focus + activation | DONE 2026-10-13: tab-reachable bindable caps, Return/Space activate, focus shows the hover highlight; W7's focus tooltip folded in. |
| W3 | `Tf2TabBar` keyboard path (A2) | DONE 2026-10-13: Left/Right wrap + activate, focus ring on the focused tab. |
| W4 | `Accessible.*` names/roles on custom items | DONE 2026-10-13 for KeyCap + the two read-only fields; the tab delegate remains (W3's scope). | Purely additive. |
| W5 | Contrast fixes | DONE 2026-10-13: `colorRed` #da7a7d, `colorBorderStrong` #59626a, cap/panel borders fixed, disabled-cap label @ 0.65; `colorTextDisabled` kept as decoration. |
| W6 | Keyboard panel scrolling via keys | DONE 2026-10-13: arrow keys scroll the Flickable (clamped, 60 px/press). |
| W7 | Cap tooltip on focus | DONE 2026-10-13 (with W2). |
| W8 | Contrast script in `tools/` | DONE 2026-10-13: `tools/contrast_check.py`, run after any `Theme.qml` change. |

- Related finding: the three empty menus (`Edit`/`View`/`Help`, A8) - removing the stubs
  or filling them is a deviation decision (see D20 in `docs/ARCHITECTURE.md`), not a bug
  fix.
- Verification: none of this is covered by `tests/tst_engine.cpp`; QML would need
  Qt Quick Test (a new dependency and build target - the same decision CI forces).
  Manual pass steps are in `docs/accessibility.md` section 4. Real screen-reader output
  is unverifiable on this machine; re-run step 4 with NVDA/Narrator/Orca before claiming
  more.

## 3. QML cleanup

- ~~Pragma sweep~~ done 2026-10-13 (`68fa1cc`): all QML files carry
  `pragma ComponentBehavior: Bound`; new files must too (AGENTS.md bar).
- 28 unqualified-access advisories (counted 2026-10-13): context-property reads
  (`cfg.loaded`, `cfg.presets`) and same-component function references (`rowWidth`,
  `fnGroupX`) qmllint cannot prove statically. All resolve at runtime; clearing them
  needs a structural change, not a pragma. Do not add new ones (AGENTS.md bar).

## 4. Input ids missing from the Edit Binds model

Confirmed bindable but not in `KeyTable.qml` (evidence and caveats in
`docs/legacy/missing-input-ids.md`):

- `joy11` .. `joy32` - generic joystick/gamepad buttons beyond the ten of the Xbox 360
  pad. Valid-but-unproven in practice; Valve's shipped config never uses them.
- `r axis pos` / `r axis neg` - joystick roll (twist) axis directions.

Parser work: none. Any new id is UI-only - the pipeline already handles quoted keys with
spaces. Do **not** surface the engine-enumerated-but-unproven ids (`break`, the lock
toggles, Novint Falcon, Steam Controller) without an in-game check - list in the legacy
doc.

## 5. Feature backlog (extension points from the QML rewrite, section 10)

Where each future feature lands, and what is already in place:

- **Type-aware argument editors** - `BindEditorDialog` "Arguments" switches on
  `TypeRole`, using `IntVar::getMinVal()`/`getMaxVal()`. The cvar metadata is parsed and
  carried by `ConsoleCommandModel` (`DefaultRole`/`MinRole`/`MaxRole`) but unused by the
  UI.
- **Alias chains** - bind a key to an alias chain; `CMD_ALIAS` detection already exists
  in `GameCfg` and `TypeNameRole` already reports `alias`.
- **Command categories** - group the `Command:` combo by a field instead of a flat list.
- **Key table owned by C++** - move `KeyTable.qml` data into a `KeyTable` QObject
  injected as a context property (per deviation D9).
- **Optional, low value**: surface the newer catalog columns (input type, platform,
  context, flags) in `CommandOverviewTab` - the one unchecked item in the
  `allcmds`-v5 migration checklist (`docs/legacy/allcmds-v5-migration.md`).
- **Config Editor tab** (`docs/DESIGN-config-editor.md`) - Chunks A and B landed
  2026-10-14: the placeholder tab is now a master-detail editor (raw text
  edit/delete/insert with save anchors, search + type filter, bind-redirect dialog,
  replay-order warnings) plus the `parseCommandLine` reverse-parse API, and the guided
  pane: cvar value editors typed by the catalog (bool checkbox, int/flt clamped to
  min/max, str token) and per-token argument editors with per-arg bounds, filled in
  both directions through `parseCommandLine`/`findCommand` (full record). Chunk A also
  surfaced a latent engine bug: `GameCfg::resetParsed()` wiped the catalog
  classification, so every loaded row typed CMD_UNKWN - alias tags now live in their
  own per-parse map. Chunks C (chain/alias editor + bind-dialog "Alias" tab) and D
  (comment rows) are pending.

## 6. Code findings from the 2026-10-13 audit

Details and fix directions in `docs/AUDIT.md`.

Items #1 and #2 from `docs/AUDIT.md` are closed by the 2026-10-13 sweep (audit annotated and scope archived there). Only open items remain:

| # | Severity | Item |
|---|----------|------|
Items #3 and #4 closed by the same 2026-10-13 sweep (audit annotated). Only data-edge
and future-feature items remain:

| 5 | nice | `cmdLex` CSV scan silently tolerates malformed quoting; data-edge note only (catalog verified clean, pinned by `catalogRowsHaveTenFields`) |
`bindtoggle` modelling (#6) is implemented (audit annotated): `bindingKinds()`, `setBindToggle()` and the dialog/cap support are now first-class.
