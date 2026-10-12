# Code Review — tf2cfg

Reviewed: 2026-10-12
Scope: `src/main.cpp`, `src/gamecfg/**`, `src/ui/**` (16 files, ~1400 lines C++).
No fixes applied. This document records findings only.

> Status after the QML rewrite (see `docs/legacy/ui-qml-rewrite.md`): findings 7–9 were in the widget UI, which now lives retired in `src/legacy/widgets/` and is not built. Finding 8 is intentionally fixed in the new UI (deviation D3); finding 6 (silent `Lexer::open` failure) is surfaced as `lastError`/D11 because `ConfigController::loadCfg` checks the file first; findings 10–11 (missing include hygiene) are fixed while keeping the engine's behaviour unchanged. Findings 1–5 and everything in `src/gamecfg/lex/**` remain untouched by design — the engine parses exactly as before, and the new UI pins the same values in the same order.

## High

| # | Location | Problem | Suggested fix |
|---|---|---|---|
| 1 | `src/gamecfg/lex/lexers.h:19-23` | `Lexer::open` lowercases the whole file (`in.readLine().toLower()`) and truncates each line at the first `//`. Case-sensitive cvar values are destroyed, and any value containing `//` (URLs, paths) is silently cut. | Read raw text. Lowercase only where the grammar is case-insensitive, or lowercase identifiers at token level. Handle comments in the lexer, not the reader. |
| 2 | `src/gamecfg/lex/cmdlexer.cpp:29-37` | `arr[0]`…`arr[4]` indexed without a size check when building the `Token`. A CSV row with fewer than 5 fields reads out of bounds; an empty `arr` reads `arr[0]` on an empty list. Undefined behavior / crash on `://cfg/allcmds.csv` drift. | Validate `arr.size() >= 5` (or fill missing fields with empty strings) before constructing the token; return `TOK_ERR` otherwise. |
| 3 | `src/gamecfg/lex/cmdlexer.cpp:10` | Local `QStringList arr;` shadows the inherited `Lexer::arr` member. Legal C++ but a trap for maintainers and a static-analysis warning. | Rename the local (e.g. `fields`), or make the base member protected with a distinct name. |
| 4 | `src/gamecfg/convars.h:39,44` | `IntVar(QString, QString, i64 def, i64 min, int max)` and `IntVar(QString, QString, int def)` take `int` where `defVal` and `Range` are `i64`. Callers at `src/gamecfg/gameconfig.cpp:66,69` pass `long long` values, so the conversion narrows silently. Large cvar ranges and defaults truncate. | Change both parameters to `cmd::i64`. |

## Medium

| # | Location | Problem | Suggested fix |
|---|---|---|---|
| 5 | `src/gamecfg/gameconfig.cpp:8-10` | `GameCfg(const QString &filename)` never calls `loadAllCmds()`, so `cmd_types` and `cmd_map` stay empty. `cmdToType()` then returns `CMD_UNKWN` for every command. Inconsistent with the default constructor. | Delegate: `GameCfg(const QString &f) : GameCfg() { appendCfg(f); }`. |
| 6 | `src/gamecfg/gameconfig.cpp:16,46` | Return value of `lex.open(...)` is ignored in `appendCfg()` and `loadAllCmds()`. A missing or unreadable file fails silently and yields an empty config with no diagnostic. | Check the return value; emit `qWarning()` / propagate an error to the UI. |
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