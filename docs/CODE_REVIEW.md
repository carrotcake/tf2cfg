# Code Review — tf2cfg

Reviewed: 2026-10-12
Scope: `src/main.cpp`, `src/gamecfg/**`, `src/ui/**` (16 files, ~1400 lines C++).
No fixes applied. This document records findings only.

> Status after the QML rewrite (see `docs/legacy/ui-qml-rewrite.md`): findings 7–9 were in the widget UI, which was deleted once the rewrite shipped. Finding 8 is intentionally fixed in the new UI (deviation D3); findings 10–11 (missing include hygiene) are fixed.
>
> **Second pass, 2026-10-12:** the QML rewrite's "the engine parses exactly as before, and the UI pins the same values" constraint is gone — it existed so the new UI could reproduce the old behaviour, and that audit is done. Findings 1, 2, 5 and 6 are now fixed in the engine (audit #6 for the quoting work; CODE_REVIEW #1 for the case fold). Findings 3, 4, 12, 13, 14, 17 remain open, and the Info items below are still worth reading before trusting a parse.

## High

| # | Location | Problem | Suggested fix |
|---|---|---|---|
| 1 | `src/gamecfg/lex/lexers.h` | ~~`Lexer::open` lowercases the whole file and truncates each line at the first `//`. Case-sensitive cvar values are destroyed, and any value containing `//` (URLs, paths) is silently cut.~~ **FIXED** — `cfgLex::preprocess` now folds only the command name (the first unquoted word) and treats `//` as a comment only outside a quoted token; `cmdLex` reads raw, so allcmds.csv descriptions are untouched. Argument case survives a load, which is what a save path needs. |
| 2 | `src/gamecfg/lex/cmdlexer.cpp` | ~~`arr[0]`…`arr[4]` indexed without a size check when building the `Token`. A CSV row with fewer than 5 fields reads out of bounds; an empty `arr` reads `arr[0]` on an empty list. Undefined behavior / crash on `://cfg/allcmds.csv` drift.~~ **FIXED** — the row is scanned field by field and padded to 10, so a short row yields empty fields instead of an out-of-bounds read. Pinned by `tests/tst_engine.cpp` (`csvShortRowIsPadded`). |
| 3 | `src/gamecfg/lex/cmdlexer.cpp:10` | Local `QStringList arr;` shadows the inherited `Lexer::arr` member. Legal C++ but a trap for maintainers and a static-analysis warning. | Rename the local (e.g. `fields`), or make the base member protected with a distinct name. |
| 4 | `src/gamecfg/convars.h:39,44` | `IntVar(QString, QString, i64 def, i64 min, int max)` and `IntVar(QString, QString, int def)` take `int` where `defVal` and `Range` are `i64`. Callers at `src/gamecfg/gameconfig.cpp:66,69` pass `long long` values, so the conversion narrows silently. Large cvar ranges and defaults truncate. | Change both parameters to `cmd::i64`. |

## Medium

| # | Location | Problem | Suggested fix |
|---|---|---|---|
| 5 | `src/gamecfg/gameconfig.cpp` | ~~`GameCfg(const QString &filename)` never calls `loadAllCmds()`, so `cmd_types` and `cmd_map` stay empty. `cmdToType()` then returns `CMD_UNKWN` for every command. Inconsistent with the default constructor.~~ **FIXED** — delegates to `GameCfg()`, so the catalog is loaded before any row is parsed. |
| 6 | `src/gamecfg/gameconfig.cpp` | ~~Return value of `lex.open(...)` is ignored in `appendCfg()` and `loadAllCmds()`. A missing or unreadable file fails silently and yields an empty config with no diagnostic.~~ **FIXED** — `appendCfg()` returns `false` (and warns) when the file cannot be read; `loadAllCmds()` warns if the resource bundle has no catalog. `ConfigController` turns that `false` into `lastError` on the GUI thread, so the UI says "file could not be read" instead of showing an empty list. |
| 7 | `src/ui/startwindow.cpp:40-48` | `changeWindow()` logic is inverted: in the `isVisible()` branch it hides `edit_window` and re-shows `this`. The function is also never connected to any signal. | **RETIRED** — widget UI not built. |
| 8 | `src/ui/startwindow.cpp:34-36` | `on_cfgPresetBox_currentIndexChanged` default branch clears `cfgPathEdit` but leaves `startGoButton` enabled, so the user can start with an empty path. | **FIXED in QML UI** (deviation D3: Go enabled exactly when path non-empty). |
| 9 | `src/ui/editwindow.cpp:50` | `KeybForm::KEYCODES[id]` is indexed with an unchecked `id` received from the signal. `KeybForm` validates before emitting, but the slot does not, so any future emitter can cause an out-of-bounds read. | **RETIRED** — widget UI not built. |
| 10 | `src/gamecfg/gameconfig.h:4` | Includes `<QApplication>` (heavy, drags in `QWidget` stack) while using `QVector` and `QHash` without including them. Compiles only through transitive includes. | Include `<QVector>` and `<QHash>`; drop `<QApplication>`. |
| 11 | `src/gamecfg/lex/lexers.h:5` | Uses `QStringList` without including `<QStringList>`. | Add the include. |

## Low

| # | Location | Problem | Suggested fix |
|---|---|---|---|
| 12 | `src/gamecfg/lex/cmdlexer.cpp:33-35` | Dead debug code: `if (line > 2099) { int a = 2 + 3; }`. | Remove. |
| 13 | `src/gamecfg/gameconfig.h:26`, `src/ui/editwindow.h:18`, `src/ui/bindeditdialog.h:17` | `const QString cmd` / `const QString filename` / `const QString str` passed by value where `const QString &` is intended. | Use `const QString &`. |
| 14 | `src/gamecfg/lex/lexers.h:56-65,107-136` | Token types returned as `const Token`, which suppresses moves; `const QString` data members make the class non-assignable. | Return by plain value and drop `const` on members. |
| 15 | `src/gamecfg/cvar.cpp` | Stub file containing only `#include "gamecfg/gameconfig.h"`. | Delete from `SOURCES`, or move the `GameCfg` implementation here instead of `gameconfig.cpp`. |
| 16 | `src/ui/startwindow.h:31` | `CFG_STRINGS` exposes only 2 of the 4 bundled cfgs; `b4nny_defbinds.cfg` and `b4nny_nobinds.cfg` are packaged in `res/res.qrc` but unreachable from the UI. | **RETIRED** — widget UI not built; presets now driven from `ConfigController::presets` which includes all four. |
| 17 | `src/gamecfg/gameconfig.cpp:99` | `printf("%s ", str.toStdString().c_str())` bypasses Qt logging and copies strings per token. | Use `qDebug()` / `qInfo()` with `qUtf8Printable()`. |

## Info — verify intent, not confirmed defects

- `src/gamecfg/lex/configlexer.cpp:22-32`: quoted tokens are returned **with** their quote characters, e.g. `"key"` rather than `key`. If `GameCfg::appendCfg` is expected to produce bare argument strings, the quotes leak into `cmd.args`.
- `src/gamecfg/lex/configlexer.cpp` + `lexers.h`: lines are stored without a separator and tokenization synthesizes a trailing `'\n'` at the end of each line. Multi-line quoted values and comment-only lines rely on this behavior; confirm it matches the intended cfg grammar.
- `src/gamecfg/lex/lexers.h:36-46`: `advanceOneChar()` increments `line` as a side effect of end-of-line, so `line`/`col` cursors are shared between `hasNext()` and tokenization. Confirm `hasNext()` remains correct if a token spans lines.

## QML UI notes (new code, not reviewed in the original pass)

The QML layer (`qml/**`, `src/ui/consolecommandmodel.h/.cpp`, `src/ui/configcontroller.h/.cpp`) was not part of this review. Known items to review when time permits:

- `ConsoleCommandModel::suggest()` rebuilds the suggestion list on every filter change — consider incremental filtering for large catalogs.
- `ConfigController::loadCfg()` does a synchronous parse on the GUI thread; large configs may jank the UI.
- `binds` QVariantMap is rebuilt from parsed rows on every `setBind`/`clearBind` — acceptable at current scale (≤60 binds), O(n) per edit.
- `KeyCap` binds read from `cfg.binds` which notifies on every change — correct but creates a property binding per cap (104 caps). Negligible at this scale.