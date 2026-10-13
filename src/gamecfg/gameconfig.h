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

    void printCfg() const;

    void loadAllCmds();

    QVector<cmd::Entry> const &getCmdList() const;

    /// Every allcmds.csv entry, with its input type, platform and context metadata.
    const QVector<cmd::ConVar *> &allCmds() const { return known_cmds; }

private:
    QHash<QString, cmd::CmdType> cmd_types;
    QHash<QString, unsigned> cmd_map;
    QVector<cmd::ConVar *> known_cmds; ///< owning; freed in dtor
    QVector<cmd::Entry> cmd_list;
    inline cmd::CmdType cmdToType(const QString &cmd) const;
};

#endif // GAMECONFIG_H
