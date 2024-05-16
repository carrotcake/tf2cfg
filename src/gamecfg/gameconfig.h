#ifndef GAMECONFIG_H
#define GAMECONFIG_H

#include <QApplication>
#include "gamecfg/convars.h"

class GameCfg {
public:
    GameCfg();

    explicit GameCfg(const QString& filename);

    void append_cfg(const QString& filename);

    void print_cfg() const;

    QVector<cmd::Entry> const& get_cmd_list() const;

private:
    QHash<QString, cmd::CmdType> all_cmds;
    QVector<cmd::Entry> cmd_list;
    cmd::CmdType cmd_to_type(const QString cmd) const;
};

#endif // GAMECONFIG_H
