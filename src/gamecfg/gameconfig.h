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

    void appendCfg(const QString &filename);

    void printCfg() const;

    void loadAllCmds();

    QVector<cmd::Entry> const &getCmdList() const;

    /// Every allcmds.csv entry, with its input type, platform and context metadata.
    const QVector<cmd::ConVar *> &allCmds() const { return known_cmds; }

private:
    QHash<QString, cmd::CmdType> cmd_types;
    QHash<QString, unsigned> cmd_map;
    QVector<cmd::ConVar *> known_cmds; ///< owning; lives as long as the app
    QVector<cmd::Entry> cmd_list;
    inline cmd::CmdType cmdToType(const QString cmd) const;
};

#endif // GAMECONFIG_H
