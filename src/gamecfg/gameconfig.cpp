#include "gameconfig.h"
#include "lex/lexers.h"

GameCfg::GameCfg() {}

GameCfg::GameCfg(const QString &filename) {
    this->appendCfg(filename);
}

void GameCfg::appendCfg(const QString &filename) {
    Lexers::cfgLex lex;
    cmd::Entry cmd;

    lex.open(filename);

    while (lex.hasNext()) {
        const auto tok = lex.nextToken();
        if (cmd.args.empty()) {
            cmd.type = cmdToType(tok.string());
        }
        if (cmd.args.size() == 1 && !cmd.args.front().compare("alias")) {
            all_cmds[tok.string()] = cmd::CMD_ALIAS;
        }
        if (tok.isEndToken()) {
            if (!cmd.args.empty()) {
                cmd_list.push_back(cmd);
                cmd.args.clear();
            }
        } else {
            cmd.args.push_back(tok.string());
        }
    }
}

inline cmd::CmdType GameCfg::cmdToType(const QString cmd) const {
    if (all_cmds.find(cmd) != all_cmds.end()) {
        return all_cmds[cmd];
    }
    return cmd::CMD_UNKWN;
}

void GameCfg::printCfg() const {
    for (const cmd::Entry &cmd : cmd_list) {
        for (const QString &str : cmd.args) {
            printf("%s ", str.toStdString().c_str());
        }
        printf(" (type: %d) \n", cmd.type);
    }
    fflush(stdout);
}
QVector<cmd::Entry> const &GameCfg::getCmdList() const {
    return cmd_list;
}
