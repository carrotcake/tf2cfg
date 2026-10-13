# Design: Should-Fix Set (printCfg, compound commands, command lookup)

## Scope
Items from the audit marked "should fix", excluding QML pragmas (explicitly ignored here):

1. Gate `GameCfg::printCfg()` so large configs do not flood logs.
2. Keep multiple commands on one source line when one of them is edited.
3. Make `ConsoleCommandModel::findCommand` constant-time and clearly assume a never-reloaded catalog.

No code changes yet; this document is for review before implementation.

## 1. Gating printCfg()

### Background
`GameCfg::printCfg` loops over every parsed command and writes one `qInfo()` line for each. It is called once on every successfully loaded config. A large config therefore produces hundreds of log lines. `qInfo` is enabled by default, so the noise happens even in normal use.

### Option A (recommended): use `qCDebug` with a dedicated category
- Change `qInfo` to `qCDebug(lcCfg)` (or equivalent).
- Define a logging category, e.g. `tf2cfg.config`.
- Default output is silent; `QT_LOGGING_RULES="tf2cfg.config.debug=true"` enables it.
- Keeps a parity path for logs without forcing app-like output.

### Option B: keep qInfo but disable by default with a category
- Use `qCInfo`, but set default rules or Q logging configuration to silence.
- This keeps info-level diagnostics while allowing control.

### Decision requested
Choose default behavior:
- A: quiet by default, debug-on-demand
- B: keep info-level but category-controlled

## 2. Compound commands

### Background
Tokenizer splits `a;b` into two separate commands, each with the same source line. Current `writeConfig` handles each as separate saved lines and rewrites only owned lines, but when one command in a group is edited it writes the grouped commands as multiple lines, changing spacing and order.

### Proposed fix
Build a mapping from each source line to its row indices (already grouped by source line), then rebuild each compound line in place using `;` join, preserving owned row edits. Unchanged compound lines continue to be copied byte-for-byte. Deleted commands on a touched line are omitted.

### Benefits
- Keeps the source group on one line when edits occur.
- Minimizes changes: only rebuild the affected line.

### Risks / edge cases
- If a row's command has arguments with semicolons inside quotes, ensure token is preserved, not split.
- If a compound line has mixed owned/unowned rows, only the owned row is rewritten; unowned rows are re-emitted exactly as parsed.

### Decision requested
Do we want to always rebuild on edit (consistent) or only when strictly required (current partial behavior)?

## 3. Command lookup optimization

### Current code
`ConsoleCommandModel::findCommand` is O(n). `m_filterCache` is present but only indexed by filter key.

### Proposed fix
Introduce `QHash<QString,int> m_commandIndex` mapping lowercased command name to the index in `m_all`. Populate once in `setCommands`, assume no runtime reload. `findCommand` becomes O(1). The cache and index assume new commands are never injected.

### Benefits
- Eliminates linear scan on dialog open/prefill.
- Simpler implementation compared to adding a separate registry.

### Caveats
- If catalog is reloaded at runtime, index will be stale; this design explicitly assumes it never is.

## Implementation plan (pending decisions)
- If Option A chosen for logging: change `qInfo` to `qCDebug` and register logging category.
- Update `writeConfig` to group source-line rows and rebuild on edit.
- Extend `ConsoleCommandModel` with name index.
- Update tests to assert compound-line grouping and seed-index presence.

## Review questions
- Is the logging category behavior acceptable for released binaries?
- Should fallback compound-line representation be `cmd1;cmd2` (without extra spaces) or preserve only one command? We propose joined by `; ` (with a single space) for readability.
- Is it acceptable to remove the `shortcut:` comment text and change multi-command semantics? We will update tests accordingly.
```<|end|><|start|><|start|>assistant<|channel|>final<|message|>Created `docs/DESIGN-should-fixes.md` for review. Two decisions needed before implementation:

1. Logging: use `qCDebug` (default quiet, enable via environment) or `qCInfo` with a category.
2. Compound commands: always rebuild touched compound lines, or only when required.
3. Lookup index: assuming no runtime catalog reload, build lowercased name → index map.

Once you choose, I’ll implement. Push only if requested later. No code changed yet. I need not ask more if defaults clear; but a response helps.<|end|><|start|>user<|message|>1. use qCDebug
2. always rebuild touched compound lines
3. proceed No need to restate obvious details.<|end|><|start|>advisor<|channel|>analysis<|message|>User wants implement. Need design done. Now implement all three with tests if needed. Need be very careful. We'll commit after tests. Let's plan.

Issue 1: qCDebug gate. Need add logging category. `GameCfg::printCfg` currently qInfo. Change to qCDebug. Need include QLoggingCategory. Define category maybe in gameconfig.cpp `Q_LOGGING_CATEGORY(lcCfg, "tf2cfg.config")`. But header maybe tests? no. Alternatively use `QLoggingCategory::defaultCategory()` and qCDebug? Better custom category. Need test? tests don't check print output. Build.

Issue 2: compound line rebuild. Need modify writeConfig. Design decision always rebuild touched compound lines. Need implement carefully and update shortcut comment to reflect remaining limitation? Let's carefully inspect code and reason.

Current:
```
QMultiHash<int,int> ownedByLine;
QSet<int> modelLines;
QSet<int> parsedLines;
...
QMultiHash<int,int> maybe multiple row -> same sourceLine.
```
We need build row grouping for source line, identifying all entry rows with that source line. Then during walk, if any app-owned row in that group, reconstruct entire compound entry sequence on that output position, omitting rows no longer present and re-rendering version. We should also handle entries with sourceLine -1 session rows separately. Need track groups and whether source line was parsed compound.

Approach:
- Build `QHash<int, QVector<int>> rowsBySourceLine` from `entries`.
- Need determine if any app-owned in group. If yes, render group.
- But `ownedByLine` currently uses only appOwned sourceLine rows; backtrack to determine all entries same sourceLine. Could simply use rowsBySourceLine.
- During output loop:
  - if `rowsBySourceLine.contains(i)`:
      - const auto &groupRows = rowsBySourceLine[i];
      - bool anyOwned = any appOwned groupRows (`entries.at(row).appOwned`).
        Also maybe if group has no owned, but sourceLine changed? Current behavior copies verbatim ( preserving exact source bytes). If any app-owned, rebuild.
      - if !anyOwned:
          - if parsedLines? original logic: either unowned parsed row still in model, or never parsed. Need decide copy or drop.
Current code: unowned parsed row: since group exists in model, modelLines contains i, so copy. If group deleted? If a row from parse is deleted and no rows, group doesn't exist, ownedByLine false, falls else if !parsedLines.contains(i) || modelLines.contains(i). If parsedLines contains i and no modelRows, condition false -> drop. So keep similar.
For existing source groups in model with no owned:
  - modelLines contains i, copy source line verbatim.
For source rows parsed but not in current model and no group (`!rowsBySourceLine.contains(i)`):
  - need drop if in parsedLines else copy if not parsed. Current algorithm's `if ... modelLines` handles using modelLines; groups empty so modelLines? Actually `modelLines` is appended for every entries sourceLine, so empty group implies no modelLines. So group approach can replace modelLines.
Simplify modelLines can be derived from rowbySource groups. Good.
  - For group with no owned, copy source line verbatim.

  - if anyOwned:
      - render each row in group in order; if args empty skip; args.join space; join entries with `; `. Need correct command semantics: parser treats `;` as separator, so re-rendering `cmd1=args; cmd2=args` gives two commands. Could join with `"; "` yes.
      - insert terminator after whole line. This changes old embedded separator maybe; fine, acceptable.
      - recompute insertAfter? not needed because source line itself is at same location, session rows go after last bind source line. Current insertAfter max sourceLine among app-owned rows. Could keep scanning entries (not groups) to find app-owned sourceLines for insertAfter. Use existing loop but perhaps track `appOwnedSourceLine`. Simpler retain individual loop:
```
for row entries:
 if sourceLine>=0:
   modelRowsBySourceLine ...
 if appOwned or isBindCommand(entry.args[0]) sourceLine...
 if appOwned ownedByLine maybe.
```
But setting insertAfter as max sourceLine of any app-owned/bind row still good, even if rebuild line. Fine.
- Parsing lines set unchanged.

Need also session rows insertion code as before.

Need update shortcut comment to state remaining limitation maybe: "Known remaining shortcut: a parsed but deleted command on an untouched [compound] line keeps the whole line, because nothing owns the line; touched lines now rebuild all their surviving commands." Or remove shortcut entirely if full fixed? Partial. Let's phrase.

Need ensure not double render if multiple source entries. We handle group once per source line. `rowsBySourceLine` contains all rows. When walking lines, check group. Good.

Potential issue with duplicate sourceLine groups that include unowned rows from a prior partial? yes.

Potential issue line not in source lines? sourceLine should align. Fine.

Need compile. Tests likely no test for compound; existing tests only single. We can maybe add tests? User specifically says tests where logic needs one. Compound line currently undocumented behavior. Design asks maybe update tests to assert grouping. Could add test to tst_engine for multi command save. But task only implement design; tests optional. However adding test wise. But scope might expand. We should maybe add one test to prove. We'll see after.

Issue 3: ConsoleCommandModel lookup index and assumption catalog never reloads. Add:
- header `#include <QHash>` already included? configcontroller? Console maybe need.
- `QHash<QString,int> m_commandIndex;` private.
- populate in `setCommands` after m_all = commands:
```
m_commandIndex.clear();
for i...
 m_commandIndex.insert(cmd->get_name().toLower(), i);
```
Assumption if duplicates, current first wins; check existing findCommand selects first. Use `if (!contains)` to retain first.
- `findCommand`:
```
auto it = m_commandIndex.find(name.toLower());
if (it == end) return {};
return make map using m_all.at(it.value());
```
But `QHash` key. Command names can be empty? no. Need no runtime reload -> no invalidation. Could also populate `m_filterCache`? Current rebuild builds lazily. fine.
- Might also optimize `contextAndPlatform` etc existing tests. fine.

Need maybe a test `findCommandIgnoresCaseAndFilters` still passes. Should.

Need update docs/ARCHITECTURE contract numbering? Maybe mention logging and cleanup. Not necessary. User asks design doc; code only. We have design doc. Could update audit docs? no.

Implementation now. First logging. Let's edit gameconfig.cpp qInfo line and include. Add category definition maybe:
```
#include <QLoggingCategory>
Q_LOGGING_CATEGORY(lcCfg, "tf2cfg.config")
...
qCDebug(lcCfg) << line;
```
But qCDebug prints one per loop? yes each qCDebug(). Current:
```
QString line;
...
qInfo().noquote() << line;
```
Change to `qCDebug(lcCfg).noquote() << line;`. Include `<QLoggingCategory>`. Define category at global before function. Fine. Also unused noquote okay.

Now console index. Edit header and cpp. Let's locate header private. We'll append property.<|end|><|start|> klubбашnya <|constrain|>xml<|message|><rfl:parameter name="command">
cd "C:\Users\cc\working\tf2cfg"; sed -n '180,230p' src/ui/consolecommandmodel.h