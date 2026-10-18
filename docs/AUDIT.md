# Repository Audit - tf2cfg (second audit)

Audit date: 2026-10-13. This is a fresh file; the 2026-10-12 audit and its closed
findings live in `docs/legacy/AUDIT.md` and are not re-raised here.

Scope: full documentation sweep for consistency against the code, plus a code read of
`src/**`, `tests/tst_engine.cpp`, `qml/**` and `Theme.qml`. Verified along the way:
the surgical save implementation matches the contract, the lexer matches the documented
tokenizer semantics, `Theme.qml` carries no duplicated/hardcoded tokens outside itself,
the contrast ratios in `docs/accessibility.md` still hold against the current palette,
and `qmllint` reports 0 errors and 28 warnings (all unqualified-access).

Documentation corrections applied during this sweep (no behavior change):

- `AGENTS.md`: `data/` holds cvar dumps, not `data/cfg/` sample configs; the tolerated
  qmllint warning count is 28 unqualified-access (no `ComponentBehavior` advisories
  remain).
- `docs/ROADMAP.md`: same qmllint count correction, plus the fact that 10 of 17 QML
  files still lack the `ComponentBehavior` pragma - the old "every file starts with the
  pragma" rule was aspirational, not true.
- `docs/ROADMAP.md`: same qmllint count correction.
- `docs/ARCHITECTURE.md`: `writeConfig(path, source, error)` parameter name; the
  `ConfigController` member table is now current here (`loadCfg` is async, `loading`,
  `consoleModel`, `presetNote` exist) - the legacy rewrite doc's section 6 predates
  those; menu text is `Save As...`; the QML pragma rule now states which files actually
  carry it; parser section cites the legacy review/audit paths and documents the
  post-`;` case nuance.
- Stale code comments: `KeyCap.qml` named "Pause" where the second non-bindable cap is
  `Menu`; `gameconfig.cpp` said the catalog is "never freed" although `~GameCfg` frees it.

## Findings

Numbered across all groups. States as in the legacy audit: **open**, or annotated when
fixed with this document.

### Must fix (bug, data loss, breaks expected use)

Nothing new at this level. The save path, the parser and the bind logic behave as their
contracts say; the two findings below are real but latent - neither is reachable through
the UI as built today.

### Should fix (correctness gap reachable via edge inputs, risky code without guard)

1. **Fixed (2026-10-13, this sweep).** `CommandListModel::setBind` in-place rebind could
   be silently overridden by a later `unbind`/`bindtoggle` row for the same key. The
   in-place path rewrote only the most recent plain `bind` row and left later rows for
   that key; `binds()` replays later-wins. Fixed by dropping every later row for the key
   on the in-place path (the append path already drops all rows for the key); a
   regression test (`setBindDropsLaterRowsForKey`) pins the behaviour.
2. **Fixed (2026-10-13, this sweep).** `ConfigController::loadCfg` had no re-entrancy
   guard. The Load button is gated on
   the path being non-empty, not on `loading` (`StartPage.qml`), so two rapid loads run
   `m_cfg.resetParsed()` + `appendCfg()` on two worker threads against the same
   `GameCfg` - a data race - and both queued GUI lambdas then run, so the older load can
   repopulate rows (and `cfgPath`/file stamps) after the newer one. Fix: return early
   while `m_loadFuture.isRunning()`, or serialize the loads.

### Nice to have

3. **Fixed (2026-10-13, this sweep).** `loadCfg` captured raw `this`; the worker and the
   queued completion now run through a `QPointer<ConfigController>` (initiating returns
   and the queued lambda no-op if the controller is destroyed), so no `invokeMethod` can
   land on a dead object. The worker parses via the weak reference the same way; loads
   still live for the app lifetime in practice.
4. **Fixed (2026-10-13, this sweep).** `cfgLex::Token` still carried `const QString str`
   / `const TokenClass type`. Finished the de-const recorded for `cmdLex::Token` (audit
   legacy CODE_REVIEW #14): members are now mutable and the string is move-constructed,
   so the token is assignable and move-friendly.
5. **`cmdLex` CSV scan tolerates malformed quoting silently**: characters between a
   closing quote and the next comma (`"abc"junk,`) are appended to the field, and a
   quoted field containing a newline would split the row (fields are scanned per
   `readLine`). The catalog is machine-generated and verified clean (all 3824 rows have
   exactly 10 fields). Guarded by the test `catalogRowsHaveTenFields`, which pins that
   shipped contract; no parser behavior changed. Still a data-edge note, not a live bug.
6. **Fixed (2026-10-13, this sweep).** `bindtoggle` was treated as a plain bind by
   `scanBinds()`. Now `binds()` still maps key → command, but `bindingKinds()` reports
   "bind"/"bindtoggle" per effective row, `setBindToggle()` authors `bindtoggle` rows,
   and the KeyCap overlay plus the bind dialog author/reopen bind toggles as such.

### Info

- **Fixed (2026-10-17).** `saveCfg()` emitted `saved(path)` before `resyncAfterSave()`;
  if the synchronous re-read failed, the success strip went out with its own error state
  unsettled. Both save paths now resync first and emit after it; success signal order
  is unobservable to any test (no spy pins it), rows are kept either way.

## Verdict

Contracts and code agree everywhere else: the surgical save, the resync, the bind
replay rules, the lexer semantics and the theme-token discipline all match
`docs/ARCHITECTURE.md` as written. Items 1 and 2 are the ones worth fixing before any
further UI work touches the bind or load paths.
