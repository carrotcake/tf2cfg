#ifndef GAMECONFIG_H
#define GAMECONFIG_H

#include <QApplication>

class GameCfg
{
public:
    typedef enum CmdType {
        CMD_CVAR,
        CMD_SVAR,
        CMD_COMMAND,
        CMD_BIND,
        CMD_ALIAS,
        CMD_UNKWN
    } CmdType;

    typedef struct Command{
        QStringList args;
        CmdType type;
        Command() : type(CMD_CVAR){};
    } Command;

    GameCfg();

    explicit GameCfg(const QString filename);

    void append_cfg(const QString filename);

    void print_cfg();

    QVector<Command> *get_cmd_list();

private:
    QHash<QString, CmdType> cmd_types;
    QVector<Command> cmd_list;
    CmdType cmd_to_type(const QString cmd);
};

#endif // GAMECONFIG_H
