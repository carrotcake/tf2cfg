#include "gameconfig.h"
#include "gamecfg/configlexer.h"

GameCfg::GameCfg() {}

GameCfg::GameCfg(const QString filename)
{
    this->append_cfg(filename);
}

void GameCfg::append_cfg(const QString filename)
{
    cfgLexer lex;
    Command cmd;

    lex.open(filename);

    while (lex.has_next()) {
        cfgLexer::Token tok = lex.next_token();
        if (cmd.args.empty()) {
            cmd.type = cmd_to_type(tok.str);
        }
        if (cmd.args.size() == 1 && !cmd.args.front().compare("alias")) {
            cmd_types[tok.str] = CMD_ALIAS;}if (tok.type == cfgLexer::TOK_END) {
            if (!cmd.args.empty()) {
                cmd_list.push_back(cmd);
                cmd.args.clear();
            }
        } else {
            cmd.args.push_back(tok.str);
        }
    }
}

GameCfg::CmdType GameCfg::cmd_to_type(const QString cmd)
{
    if (cmd_types.find(cmd) != cmd_types.end())
        return cmd_types[cmd];
    return CMD_UNKWN;
}

void GameCfg::print_cfg()
{
    for (Command &cmd : cmd_list) {
        for (QString &str : cmd.args)
            printf("%s ", str.toStdString().c_str());
        printf(" (type: %d) \n", cmd.type);
    }
    fflush(stdout);
}
const QVector<GameCfg::Command> &GameCfg::get_cmd_list() const
{
    return cmd_list;
}
