# Design: Should-Fix Set (printCfg, compound commands, command lookup)

Status: **implemented** (decisions [user]: qCDebug; always rebuild touched compound
lines; proceed with the lookup index). The sections below record what was decided;
the current code and its tests are the authority.

## 1. Gating printCfg()

`GameCfg::printCfg` looped over every parsed command and wrote one `qInfo()` line per
command on every successful load - hundreds of log lines for a large config, `qInfo`
being on by default.

**Done**: dedicated logging category `tf2cfg.config`
(`Q_LOGGING_CATEGORY(lcCfg, "tf2cfg.config")`, gameconfig.cpp) and `qCDebug(lcCfg)` in
`printCfg` - silent by default, enable with
`QT_LOGGING_RULES="tf2cfg.config.debug=true"`.

## 2. Compound commands

`writeConfig` used to write each command of a `;`-compound source line as separate
lines when one of them was edited, changing spacing and order.

**Done** (2026-10-14, Config Editor chunks C/D): touched lines rebuild as ONE line -
rows grouped by source line (session compounds by `groupId`), joined with `; `, the
line's comment tail re-emitted once after the last surviving command. Untouched lines
still copy byte-for-byte. Semicolons inside quoted tokens are never split (the lexers
stay the only quote authority). See `docs/DESIGN-config-editor.md` §5/§6.

## 3. Command lookup optimization

`ConsoleCommandModel::findCommand` was O(n) per call.

**Done**: `QHash<QString, int> m_commandIndex` (lowercased name -> index into `m_all`),
populated once in `setCommands`; `findCommand` is O(1) and explicitly assumes the
catalog is never reloaded at runtime (same assumption the filter cache makes).