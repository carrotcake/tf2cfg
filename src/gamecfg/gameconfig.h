#ifndef GAMECONFIG_H
#define GAMECONFIG_H

#include <QApplication>
#include "gamecfg/convars.h"

class GameCfg {
public:
    GameCfg();

    explicit GameCfg(const QString &filename);

    void appendCfg(const QString &filename);

    void printCfg() const;

    void loadAllCmds();

    QVector<cmd::Entry> const &getCmdList() const;

private:
    QHash<QString, cmd::CmdType> cmd_types;
    QHash<QString, unsigned> cmd_map;
    QVector<cmd::ConVar> known_cmds;
    QVector<cmd::Entry> cmd_list;
    inline cmd::CmdType cmdToType(const QString cmd) const;
};

#endif // GAMECONFIG_H
