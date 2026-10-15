# Roadmap - tf2cfg

The single list of open work. Fixed, implemented or deliberately dismissed items do not
appear here; their records live in `docs/legacy/` (audit, code review, migration notes)
and stay there.

Order within each section is priority order. Open code findings from the 2026-10-13
audit (`docs/AUDIT.md`) live in section 6; dismissed items are not listed here at all.

## 1. CI workflow (the 2026-10-12 audit's only open finding, legacy #22)

DONE 2026-10-13: `.github/workflows/ci.yml` — Linux only (`ubuntu-latest`), on push to
`qml-rewrite`/`master` and on PRs. Jobs: install Qt 6 apt packages, build the app,
build + run `tf2cfg_test` (offscreen platform), gate `qmllint` at 0 errors and 0
unqualified-access warnings (the printed count is the bar, AGENTS.md), run
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
items (W1-W8). W1-W8 are done (2026-10-13) and the screen-reader pass ran 2026-10-15 with
Microsoft Narrator: all five steps pass now, after the one defect it found (the tab strips
never announced their tab names) took two fixes in `Tf2TabBar`.

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
  non-bindable cap stays out of the tab chain, focus shows the highlight, the tab strip
  reports the next/previous tab with wrapping on Left/Right (W3), and the strip announces
  as a tab list with named page tabs (W4). The 2026-10-15 Narrator pass:
  - Step 1 (start screen), 3 (cap focus + bind dialog opens), 5 (bound cap reads its bind
    as the description): pass.
  - Steps 2 and 4 (both tab strips) took two rounds: with `Accessible.PageTabList` on the
    bar the reader said "Tab contains 3 items" - list recognised, names still missing -
    because the strip itself held the focus and a reader announces whatever holds it.
    `Tf2TabBar` is now a `FocusScope` whose current tab takes the focus, which is Qt's own
    TabBar structure; the strip stays a single Tab stop and Left/Right still switch
    (pinned by `test_tabBarAccessibleRoles` and `test_tabBarFocusFollowsTheCurrentTab`).
  - Round 3 confirmed the names ("Command Overview, tab, 1 of 3, selected"), so all five
    steps of the pass have run with a real reader. Re-run it after any `Accessible.*`,
    focus or tab-order change.
  - Manual pass steps stay in `docs/accessibility.md` section 4 - the tests prove the
    handlers fire and the properties are set, not what a reader says.

## 3. QML cleanup

- ~~Pragma sweep~~ done 2026-10-13 (`68fa1cc`): all QML files carry
  `pragma ComponentBehavior: Bound`; new files must too (AGENTS.md bar).
- unqualified-access advisories: **0, enforced** (2026-10-15). The backend used to reach QML
  as the `cfg`/`keyTable` context properties, which no QML tool can resolve, so every read
  was an unqualified access (47 on Qt 6.12) and the tree worked around it with `cfgRef`
  hoists. It is now the `tf2cfg` QML module - `Cfg` and `KeyTable` singletons registered from
  `src/ui/*.h` by qmltyperegistrar, used as `Cfg.consoleModel` / `KeyTable.keyLabel(…)` - so
  the accesses are qualified, type-checked and one `.qmltypes` away from completion in an
  editor. The migration also fixed what the checker then exposed: three model properties are
  declared as their concrete types (`ConsoleCommandModel *consoleModel`) instead of
  `QAbstractItemModel *`, the four published pixel sizes are `double` rather than `qreal`, the
  key table's lists are `QList<QVariant>` (`list<var>` - `QVariantList` hides `.length`), and
  two parent-element reads are qualified with their id. The CI lint step lints with
  `-I qmlmodules` (the module directory qmake assembles) and fails on any nonzero count, so
  the bar is now a gate rather than a printed number. The step lints `tests/qml/` alongside
  `qml/` since 2026-10-15: the test tree is QML too, and it had accumulated three
  `missing-property` warnings (`.length`/`.forEach` on a `QVariantList`) that no gate saw
  while only `qml/` was linted.
- Usage-signature parsing: **one parse, four readers** (2026-10-15). `[str] <flt>` was being
  taken apart four times - in `loadArgBounds` to count arguments, in `ConsoleCommandModel`'s
  `argsSpec` role to derive `optional` and `isFloat`, and once in each of
  `BindEditorDialog.qml` and `ConfigEditorTab.qml` to build the argument editors. It is now
  `cmd::parseSignature` (`src/gamecfg/`, beside `ArgSpec`), reached from QML as
  `Cfg.parseSignature` returning one `{kind, optional}` per argument (`QList<QVariant>` in the
  header, so QML and the tools see `list<var>` rather than a `QVariantList` whose array members
  they cannot resolve); the other three readers go through it, so they cannot disagree about an
  argument's index or its signed/optional shape. The struct's `bare` flag carries what the
  role's old surface check meant (a cvar's signature is a value type, not an argument).
  `BindEditorDialog`'s argument-editor rows are pinned from the outside by
  `test_bindDialogBuildsOneEditorPerSignatureArgument`: it opens the dialog for a key, selects
  `callvote` ("[str] <str>") in the dropdown the way a user would, and asserts the rendered
  editors - both labels from the parsed kinds, the include checkbox on the optional argument
  only. The parse itself is pinned by `EngineTest::signatureParsesIntoEditorArguments`,
  `EngineTest::argsSpecRoleCarriesPerArgumentBounds` and
  `test_parseSignatureReachesQmlAsAnArgumentList`.

## 4. Input ids missing from the Edit Binds model

**Closed 2026-10-15** (in-game run, `docs/in-game-checklist.md` §2). Both candidate
groups bind silently - `bind joy11 +attack` and `bind "r axis pos" "+attack"` print no
error - but neither fired with real hardware (an Xbox-style pad for `joy11..joy32`, a USB
flightstick for the twist axis). The engine takes key names without validating them, so
"no error" proves nothing on its own; with no id observed firing, nothing entered the
keyboard model. Evidence and the engine enumeration remain in
`docs/legacy/missing-input-ids.md`. The engine-enumerated ids kept out of the UI (`break`,
the lock toggles, Novint Falcon, Steam Controller) were not part of the run and stay out.

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

## 7. Catalog fidelity (closed 2026-10-15)

The 2026-10-15 in-game run showed the catalog is not a faithful mirror of the live client:
`grab_window` answers **Unknown command** although the dump that fed the catalog
(`data/cvarlist_linux.txt`) lists it, and `sv_pure` carried another command's description.
The repo owner then produced a dump from their own client -
`data/tf2-cvarlist-10-26-gamedump-w64.txt`, 3656 names, Windows, one session - and
`tools/catalog_diff.py` runs the comparison: type drift, `cheat` drift, description drift,
dead-row and new-row candidates, one count per category with the full lists. It cannot see
the input signature, the platform or the context column: no dump carries them, so those stay
authored (`tools/field_dicts.py`, `res/cfg/schema.md`).

Applied from the dump:

- **58 rows whose type field said `cvar`** - `res/cfg/schema.md` allows only `cmd`, `ncmd` or
  empty, and `cmdLex::Token::isCmd()` asks just whether the field is non-empty, so those 58
  console variables loaded as *commands*: they sat under the editor's Commands filter and got
  a command editor in the guided pane. They are variables again (2831 variables / 1015 `cmd`
  / 34 `ncmd`), pinned by `EngineTest::catalogTypeFieldIsEmptyOrCommand`.
- **5 descriptions corrected** - four typos (`maxmimum`, `memebers`, a vote quorum minimum of
  "0.5") and one sentence the client has rewritten (`sv_master_share_game_socket`).
- **56 rows added** - 36 cvars and 20 commands the client has and the catalog lacked
  (`tf_voice_command_suspension_*`, `tf_ready_countdown_*`, `vgui_ui_scale_*`, the `snd_*`
  speaker-positioning commands, ...). Type, default, flags and help text come from the dump;
  the input type and the context follow `res/cfg/schema.md` (a scale factor or a rate is a
  float although the client prints it without a decimal point, the call the file already makes
  for `sensitivity`), and a command whose usage line nobody has seen keeps the signature
  `none`. Pinned by `EngineTest::catalogRowsAddedFromTheLiveDump`.

Explained, not drift: 224 catalog rows are absent from the dump. 138 of them are hidden-flagged
(104 of the 109) or `ncmd` (34 of 34), and `cvarlist` lists neither. 45 more are tagged `linux`
- ToGL internals a Windows client does not register, though three `gl_*` names do appear, so the
platform tags are not perfect either - and 5 are `xbox`. That leaves 36 the client should have
registered: 34 universal rows and the 2 tagged `windows`. Accepted drift, both settled in-game
on 2026-10-15: `disguise` keeps the usage string the console printed, and `sv_pure` keeps its
rule list even though the client's own help for it is another command's ("Show user data.").

Closed the same day by one console session: `data/in-game-checks-2026-10-15.cfg`
(`docs/in-game-checklist.md` §4, log in `data/checks.txt`) answered the 36 rows whose absence
hidden/ncmd and the platform tags did not explain, the 20 new commands' usage lines, and the
13 cheat markers. Its answers:

- **35 of the 36 rows are gone** ("Unknown command") and left the catalog; `r_spewleaf`
  answered with a value although `cvarlist` never lists it, so its row stayed. An absent name
  is a candidate, never proof - the same reason the 104 hidden rows were not touched.
- **The 20 commands all ran bare without printing a usage line**, so `none` stands as their
  signature. The twelve `snd_*` speaker commands report their setting when run bare
  (`snd_front_headphone_position = 90.000000`), which is why the dump registers them as
  commands rather than variables.
- **All 13 `cheat` tags came off.** Each cvar was set away from its dump value with
  `sv_cheats 0` and none was refused. The wiki and CS2's list carry the tag, but both TF2 dumps
  and the run agree the client does not enforce it. A `cheat` tag the dump did report stays.

Four of the 35 removed rows (`sdl_double_click_size`, `sdl_double_click_time`,
`mat_autoload_glshaders`, `mat_autosave_glshaders`) look Linux-only by name - SDL input, ToGL
shaders - so a `cvarlist` from the Linux build would bring them back with a `linux` tag.
