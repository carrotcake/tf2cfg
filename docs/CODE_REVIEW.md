# Code Review — tf2cfg

Reviewed: 2026-10-12
Scope: `src/main.cpp`, `src/gamecfg/**`, `src/ui/**` (16 files, ~1400 lines C++).
No fixes applied. This document records findings only.

> Status after the QML rewrite (see `docs/legacy/ui-qml-rewrite.md`): findings 7–9 were in the widget UI, which was deleted once the rewrite shipped. Finding 8 is intentionally fixed in the new UI (deviation D3); findings 10–11 (missing include hygiene) are fixed.
>
> **Status, 2026-10-12 (second pass).** The QML rewrite's "the engine parses exactly as before, and the UI pins the same values" constraint is gone — it existed so the new UI could reproduce the old behaviour, and that audit is done. Every finding below is now fixed or retired: 1, 2, 3, 4, 5, 6, 10, 11, 12, 13, 14, 15 and 17 are fixed in the engine, 8 is fixed in the QML UI (D3), and 7, 9 and 16 died with the widget UI. The parser and the bind logic have tests in `tests/tst_engine.cpp`; the three Info items at the bottom are answered there rather than left open.

## High

| # | Location | Problem | Suggested fix |
|---|---|---|---|
| 1 | `src/gamecfg/lex/lexers.h` | ~~`Lexer::open` lowercases the whole file and truncates each line at the first `//`. Case-sensitive cvar values are destroyed, and any value containing `//` (URLs, paths) is silently cut.~~ **FIXED** — `cfgLex::preprocess` now folds only the command name (the first unquoted word) and treats `//` as a comment only outside a quoted token; `cmdLex` reads raw, so allcmds.csv descriptions are untouched. Argument case survives a load, which is what a save path needs. |
| 2 | `src/gamecfg/lex/cmdlexer.cpp` | ~~`arr[0]`…`arr[4]` indexed without a size check when building the `Token`. A CSV row with fewer than 5 fields reads out of bounds; an empty `arr` reads `arr[0]` on an empty list. Undefined behavior / crash on `://cfg/allcmds.csv` drift.~~ **FIXED** — the row is scanned field by field and padded to 10, so a short row yields empty fields instead of an out-of-bounds read. Pinned by `tests/tst_engine.cpp` (`csvShortRowIsPadded`). |
| 3 | `src/gamecfg/lex/cmdlexer.cpp` | ~~Local `QStringList arr;` shadows the inherited `Lexer::arr` member.~~ **FIXED** — renamed to `fields`. |
| 4 | `src/gamecfg/convars.h` | ~~`IntVar(QString, QString, i64 def, i64 min, int max)` and `IntVar(QString, QString, int def)` take `int` where `defVal` and `Range` are `i64`, so large defaults and bounds narrow silently.~~ **FIXED** — both parameters are `i64`; `FltVar` takes `double` throughout. |

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
| 12 | `src/gamecfg/lex/cmdlexer.cpp` | ~~Dead debug code: `if (line > 2099) { int a = 2 + 3; }`.~~ **FIXED** — removed. |
| 13 | `src/gamecfg/gameconfig.h` | ~~`const QString cmd` / `const QString filename` / `const QString str` passed by value where `const QString &` is intended.~~ **FIXED** — `cmdToType` takes a reference; the other two instances were in the widget UI, now deleted. |
| 14 | `src/gamecfg/lex/lexers.h` | ~~Token types returned as `const Token`, which suppresses moves; `const QString` data members make the class non-assignable.~~ **FIXED** — the `const` members went with audit #5 and both `nextToken()` now return a plain `Token`, so a row's 10 `QString`s move instead of being copied. |
| 15 | `src/gamecfg/cvar.cpp` | ~~Stub file containing only `#include "gamecfg/gameconfig.h"`.~~ **FIXED** — deleted along with its `SOURCES` entry. |
| 16 | `src/ui/startwindow.h` | ~~`CFG_STRINGS` exposes only 2 of the 4 bundled cfgs; `b4nny_defbinds.cfg` and `b4nny_nobinds.cfg` are packaged in `res/res.qrc` but unreachable from the UI.~~ **RETIRED** — widget UI deleted; all four presets are in `ConfigController::presets`. |
| 17 | `src/gamecfg/gameconfig.cpp` | ~~`printf("%s ", str.toStdString().c_str())` bypasses Qt logging and copies strings per token.~~ **FIXED** — `printCfg` writes through a `QTextStream` on `stdout`, so the P12/D7 parity dump still lands on stdout while the QString is encoded in place as UTF-8. Verified byte-identical output. |

## Info — answered against the behaviour as built

- **Quoted tokens keep their quote characters**, e.g. `"key"` rather than `key`. Intentional: a token is the line as written, the Command Overview shows the line verbatim (P38/D16), and `CommandListModel::unquoted()` is the single place that strips them and reads `\"`/`\\`. Pinned by `setBindEscapesQuotesInRow`.
- **Lines are stored without a separator and tokenization synthesizes a trailing `'\n'`.** That synthesized character is how the lexer recognises end-of-line without a sentinel in the buffer. A quoted token therefore cannot span two lines: an unterminated quote returns `TOK_ERR`, which is the same line-oriented grammar the game uses.
- **`advanceOneChar()` increments `line` as a side effect of end-of-line, so `hasNext()` and tokenization share the cursor.** Still correct, because no token spans a line (above), so tokenization always lands the cursor on a line boundary before `hasNext()` is asked again. The tokenizer tests cover the line-by-line cases.

## QML UI notes (new code, reviewed 2026-10-12)

All four items came out clean, and the first three were fixed along the way:

- ~~`ConsoleCommandModel::suggest()` rebuilds the suggestion list on every filter change.~~ **FIXED** — `suggest()` walks the per-combo cached visible indices (audit #4). Covered by `suggestOnlyOffersVisibleNames`.
- ~~`ConfigController::loadCfg()` does a synchronous parse on the GUI thread.~~ **FIXED** — parsing runs on a `QtConcurrent` worker and the model is swapped on the GUI thread (audit #3).
- ~~`binds` QVariantMap is rebuilt from parsed rows on every `setBind`/`clearBind`.~~ **FIXED** — cached and invalidated on change (audit #2).
- `KeyCap` binds read from `cfg.binds`, which notifies on every change: one property binding per cap, 104 caps, negligible at this scale. No action.