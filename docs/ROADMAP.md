# Roadmap - tf2cfg

The single list of open work. Fixed, implemented or deliberately dismissed items do not
appear here; their records live in `docs/legacy/` (audit, code review, migration notes)
and stay there.

Order within each section is priority order. Open code findings from the 2026-10-13
audit (`docs/AUDIT.md`) live in section 6; dismissed items are not listed here at all.

## 1. CI workflow (the 2026-10-12 audit's only open finding, legacy #22)

DONE 2026-10-13: `.github/workflows/ci.yml` — Linux only (`ubuntu-latest`), on push to
`qml-rewrite`/`master` and on PRs. Jobs: install Qt 6 apt packages, build the app,
build + run `tf2cfg_test` (offscreen platform), gate `qmllint` at 0 errors and print
the unqualified-access warning count (the printed count is the bar, AGENTS.md), run
`tools/contrast_check.py`. The app binary is only
compiled, never launched (no display); the test binary runs headless via
`QT_QPA_PLATFORM=offscreen`. First Linux compile of this codebase — the CI run is the
verification.

DONE 2026-10-14, same workflow: AppImage packaging. `linuxdeploy` + `linuxdeploy-plugin-qt`
bundle the built binary with Qt 6.4.2 libs/plugins (QML modules come from the
`qml6-module-*` runtime packages); the icon is rendered at build time with ImageMagick
from the repo's TF2 font (`res/icon/tf2cfg.svg` is the mockup, `tf2cfg.desktop` the
menu entry). The AppImage is smoke-tested headless (offscreen run for 15 s, exit 124 =
success). 2026-10-14, later: packaging moved to `release.yml` (tag-only `v*` trigger,
attaches the AppImage to a GitHub Release via `softprops/action-gh-release@v3`) because
`paths` filters are unreliable on tag pushes; ci.yml keeps build + tests + lint +
contrast and skips entirely on docs-only pushes (`paths-ignore: ["docs/**", "**.md"]`,
plus `workflow_dispatch` for manual runs). Compatibility note: the AppImage
targets current distros (glibc 2.39 / Qt 6.4.2 floor) — accepted, per project decision.

DONE 2026-10-14, later: `release.yml` gained a parallel `windows` job (MinGW, the dev
box's toolchain): the exe icon is rendered at build time with the same ImageMagick
font script and packed into a multi-size `.ico` (`icon:auto-resize`, never committed),
baked into the binary via `res/icon/tf2cfg.rc` + `win32: RC_FILE` (qmake/windres,
verified locally), deployed with `windeployqt --qmldir qml`, smoke-tested headless
(15 s alive = success, AppImage parity), and attached to the same Release as
`TF2-Config-Manager-windows-x64.zip`. install-qt-action pins an aqtinstall upstream
commit: aqt 3.3.0 cannot resolve Qt ≥6.11 Windows desktop metadata (per-arch package
folders since 6.11; miurahr/aqtinstall#1000) — revisit on the next aqtinstall
release, fallback `version: '6.10.*'` without the pin.

DONE 2026-10-14, later: the first release run exposed two artifact bugs, both now
guarded. (1) The build linked with the runner's preinstalled gcc 15.2, not Qt's
bundled MinGW (`__imp___argc` undefined from `libQt6EntryPoint.a` - Qt's prebuilt libs
are built against their bundled runtime). The toolchain now comes from
install-qt-action's `tools: 'tools_mingw1310'` input (aqt installs GCC 13.1 under
`<Qt root>/Tools/mingw1310_64` and the action puts it on PATH), and the build step
fails loudly unless the compiler is GCC 13. (2) The published zip contained no exe:
the archive step's PowerShell filter
`$_.Name -match '\.(exe|dll)$' -or $_.PSIsContainer -and $_.Name -in @(...)` parses as
`((is a file) -or (is a dir)) -and (name in the list)` - `-or` and `-and` have equal
precedence and evaluate left to right - so only the named plugin directories survived
and every top-level file (the exe and 46 Qt6 DLLs) was dropped. The job now stages the
exe in a clean `tf2cfg/` dir, deploys into it with windeployqt, zips that dir, and
asserts `tf2cfg/tf2cfgreal.exe` + `Qt6Core.dll` + `Qt6Qml.dll` are in the archive. The
smoke test also tests the bundle rather than the runner: `--include-plugins qoffscreen`
(windeployqt ships only `qwindows.dll`, and an offscreen run without the plugin dies
before the event loop), `QT_PLUGIN_PATH`/`QML2_IMPORT_PATH` cleared and
`PATH=System32` so the install's runtime cannot mask a bundle gap, plus a check for
Qt's platform-plugin fatal text (a blocking fatal dialog keeps the process alive, so
"still alive after 15 s" alone proved nothing).

## 2. Accessibility (`docs/accessibility.md`)

The gap was real: the app is keyboard-usable up to the keyboard panel, then stops - the
104 key caps and the three editor tabs are `MouseArea`-only, and nothing carries
`Accessible.*`. `docs/accessibility.md` holds the findings (A1-A8) and the sized work
items (W1-W8). W1-W8 are done (2026-10-13); still open: the screen-reader pass (section
4, needs a real screen reader).

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

- A8 decided 2026-10-14: the three empty menus (`Edit`/`View`/`Help`) stay. D20 in
  `docs/ARCHITECTURE.md` holds the rule - `File` carries the save actions, the other
  menus stay empty until something belongs in them. Not a bug and not open work.
- Verification: the engine suite (`tests/tst_engine.cpp`) still covers none of this, but
  the QML suite (`tests/qml/`, Qt Quick Test, added 2026-10-15) now pins the kernels those
  work items added: a bindable cap takes focus and activates on Return/Space (W2), a
  non-bindable cap stays out of the tab chain, focus shows the highlight, and the tab strip
  reports the next/previous tab with wrapping on Left/Right (W3). Manual pass steps stay in
  `docs/accessibility.md` section 4 - the tests prove the handlers fire, not that the UI
  looks right. Real screen-reader output is still unverifiable on this machine; re-run
  step 4 with NVDA/Narrator/Orca before claiming more - the runnable version of that pass is
  `docs/in-game-checklist.md` §3.

## 3. QML cleanup

- ~~Pragma sweep~~ done 2026-10-13 (`68fa1cc`): all QML files carry
  `pragma ComponentBehavior: Bound`; new files must too (AGENTS.md bar).
- unqualified-access advisories: the CI lint step prints `unqualified-access warnings: N`
  every run - the printed count is the authority (0 as of 2026-10-14), never add new ones
  (AGENTS.md bar). The two linters count differently: the local qmllint (Qt 6.12) reports
  ~48 `Unqualified access [unqualified]` lines across the tree (context-property `cfg`
  reads it cannot resolve), while CI's `qmllint-qt6` (Qt 6.4) prints 0 for the same tree.
  The CI number is the bar; locally, keep a file's count at or below what it was. New QML
  does that by hoisting the context property into a root property
  (`readonly property var cfgRef: cfg`) and reading the local id from nested delegates.

## 4. Input ids missing from the Edit Binds model

Confirmed bindable but not in the key table (evidence and caveats in
`docs/legacy/missing-input-ids.md`):

- `joy11` .. `joy32` - generic joystick/gamepad buttons beyond the ten of the Xbox 360
  pad. Valid-but-unproven in practice; Valve's shipped config never uses them.
- `r axis pos` / `r axis neg` - joystick roll (twist) axis directions.

Parser work: none. Any new id is UI-only - the pipeline already handles quoted keys with
spaces. Do **not** surface the engine-enumerated-but-unproven ids (`break`, the lock
toggles, Novint Falcon, Steam Controller) without an in-game check - list in the legacy
doc. The in-game run to settle these is written out in `docs/in-game-checklist.md` §2.

## 5. Feature backlog (extension points from the QML rewrite, section 10)

Where each future feature lands, and what is already in place. Nothing in this section is
open: both extension points the rewrite left behind have landed, so treat this as the
record of where a new feature would go.

- **DONE 2026-10-15**: the newer catalog columns surface on `CommandOverviewTab`'s rows -
  input signature, non-universal platform, context, and a `cheat` marker, with a header
  strip naming them - the last unchecked item of the `allcmds`-v5 migration
  (`docs/legacy/allcmds-v5-migration.md`). Narrowed from the item's wording: the raw FCVAR
  flag list is not shown (near-identical on most rows, developer detail); its actionable
  part, `cheat`, is the marker. The columns come from `cfg.consoleModel.findCommand()`
  per row, the same lookup the Config Editor's guided pane uses.
- **Config Editor tab** (`docs/DESIGN-config-editor.md`) - **complete** (Chunks A-D,
  2026-10-14): master-detail editor (raw text edit/delete/insert with save anchors,
  search + type filter incl. comments, bind-redirect dialog, replay-order warnings),
  guided pane (catalog-typed editors, reverse-parse fill), chain editor (compound
  lines and alias bodies as segment lists; BindEditorDialog "Alias" tab), and comment
  rows (standalone `CMD_COMMENT` rows + trailing `commentTail`, editable, save
  round-trips them). Chunk A surfaced a latent engine bug: `GameCfg::resetParsed()`
  wiped the catalog classification, so every loaded row typed CMD_UNKWN - alias tags
  now live in their own per-parse map.

## 6. Code findings from the 2026-10-13 audit

Details and fix directions in `docs/AUDIT.md`. All findings are closed or annotated
there: #1/#2/#3/#4 fixed in the 2026-10-13 sweep, #5 (`cmdLex` CSV malformed quoting)
is a data-edge note only (catalog verified clean, pinned by
`catalogRowsHaveTenFields`), #6 (`bindtoggle` modelling) is implemented as
first-class support. Nothing open.
