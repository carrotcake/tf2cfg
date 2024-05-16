#include "gameconfig.h"
#include "gamecfg/configlexer.h"

GameCfg::GameCfg() {}

GameCfg::GameCfg(const QString &filename) {
    this->append_cfg(filename);
}

void GameCfg::append_cfg(const QString &filename) {
    cfgLexer lex;
    cmd::Entry cmd;

    lex.open(filename);

    while (lex.has_next()) {
        const cfgLexer::Token tok = lex.next_token();
        if (cmd.args.empty()) {
            cmd.type = cmd_to_type(tok.str);
        }
        if (cmd.args.size() == 1 && !cmd.args.front().compare("alias")) {
            all_cmds[tok.str] = cmd::CMD_ALIAS;
        }
        if (tok.type == cfgLexer::TOK_END) {
            if (!cmd.args.empty()) {
                cmd_list.push_back(cmd);
                cmd.args.clear();
            }
        } else {
            cmd.args.push_back(tok.str);
        }
    }
}

cmd::CmdType GameCfg::cmd_to_type(const QString cmd) const {
    if (all_cmds.find(cmd) != all_cmds.end()) {
        return all_cmds[cmd];
    }
    return cmd::CMD_UNKWN;
}

void GameCfg::print_cfg() const {
    for (const cmd::Entry &cmd : cmd_list) {
        for (const QString &str : cmd.args) {
            printf("%s ", str.toStdString().c_str());
        }
        printf(" (type: %d) \n", cmd.type);
    }
    fflush(stdout);
}
QVector<cmd::Entry> const &GameCfg::get_cmd_list() const {
    return cmd_list;
}
