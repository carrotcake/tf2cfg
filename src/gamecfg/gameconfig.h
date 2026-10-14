#ifndef GAMECONFIG_H
#define GAMECONFIG_H

#include <QHash>
#include <QString>
#include <QVector>

#include "gamecfg/convars.h"

class GameCfg {
public:
    GameCfg();

    explicit GameCfg(const QString &filename);

    ~GameCfg();

    /// Parse a config file into the command list. False when the file cannot be read, so
    /// the caller can report it instead of showing an empty list (review finding 6).
    bool appendCfg(const QString &filename);

    /// Drop the parsed rows and the alias types the last parse collected. The command
    /// catalog (allcmds.csv) is untouched - call before parsing a different file, since
    /// appendCfg accumulates and a second parse would duplicate every row.
    void resetParsed();

    void printCfg() const;

    void loadAllCmds();

    /// Attach per-argument authoring bounds from the argbounds companion file
    /// (docs/DESIGN-arg-bounds.md). Optional file: a missing one leaves every command
    /// unbounded. Parameterized so tests can point it at a temp file.
    void loadArgBounds(
        const QString &path = QStringLiteral("://cfg/argbounds.csv"));

    QVector<cmd::Entry> const &getCmdList() const;

    /// Catalog + parsed-alias classification for a command head (docs/DESIGN-config-editor.md
    /// §8). cmd_types holds every allcmds.csv name; alias_types the names the last
    /// parse defined. A head defined only by another cfg (executed at runtime) is
    /// CMD_UNKWN.
    cmd::CmdType lookupType(const QString &head) const {
        const QString key = head.toLower();
        auto alias = alias_types.constFind(key);
        if (alias != alias_types.cend())
            return alias.value();
        return cmd_types.value(key, cmd::CMD_UNKWN);
    }

    /// Every allcmds.csv entry, with its input type, platform and context metadata.
    const QVector<cmd::ConVar *> &allCmds() const { return known_cmds; }

private:
    QHash<QString, cmd::CmdType> cmd_types;
    /// Alias names the current parse has defined (appendCfg). Kept apart from
    /// cmd_types so resetParsed() can drop them per-parse without touching the
    /// catalog classification - previously the catalog types were wiped on every
    /// load and every row typed CMD_UNKWN (found by the Config Editor's tests,
    /// docs/DESIGN-config-editor.md chunk A).
    QHash<QString, cmd::CmdType> alias_types;
    QHash<QString, unsigned> cmd_map;
    QVector<cmd::ConVar *> known_cmds; ///< owning; freed in dtor
    QVector<cmd::Entry> cmd_list;
    inline cmd::CmdType cmdToType(const QString &cmd) const;
};

#endif // GAMECONFIG_H
