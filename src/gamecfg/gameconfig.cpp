#include "gameconfig.h"
#include "lex/lexers.h"

GameCfg::GameCfg() {
    loadAllCmds();
}

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
            cmd_types[tok.string()] = cmd::CMD_ALIAS;
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
    if (cmd_types.find(cmd) != cmd_types.end()) {
        return cmd_types[cmd];
    }
    return cmd::CMD_UNKWN;
}

void GameCfg::loadAllCmds() {
    Lexers::cmdLex lex;
    lex.open("://cfg/allcmds.csv");
    while (lex.hasNext()) {
        const auto tok = lex.nextToken();
        if (tok.isCmd()) {
            //ConCmd
            known_cmds.push_back(cmd::ConCmd(tok.name(), tok.docStr(), ""));
            cmd_map.insert(tok.name(), known_cmds.size() - 1);
            cmd_types.insert(tok.name(), cmd::CMD_CMD);
            //handle bind

            continue;
        }
        if (tok.hasDefault()) {
            bool ok;
            const auto idefval = tok.defVal().toLongLong(&ok);
            ok = ok && !tok.defVal().contains('.');
            if (ok) {
                //intvar
                if (tok.hasRange()) {
                    const auto imin = tok.minVal().toLongLong(), imax = tok.maxVal().toLongLong();
                    known_cmds.push_back(cmd::IntVar(tok.name(), tok.docStr(), idefval, imin, imax));

                } else {
                    known_cmds.push_back(cmd::IntVar(tok.name(), tok.docStr(), idefval));
                }
                cmd_map.insert(tok.name(), known_cmds.size() - 1);
                cmd_types.insert(tok.name(), cmd::CMD_IVAR);
                continue;
            }
            const auto fdefval = tok.defVal().toDouble(&ok);
            if (ok) {
                //fltvar
                if (tok.hasRange()) {
                    const auto fmin = tok.minVal().toDouble(), fmax = tok.maxVal().toDouble();
                    known_cmds.push_back(cmd::FltVar(tok.name(), tok.docStr(), fdefval, fmin, fmax));
                } else {
                    known_cmds.push_back(cmd::FltVar(tok.name(), tok.docStr(), fdefval));
                }
                cmd_map.insert(tok.name(), known_cmds.size() - 1);
                cmd_types.insert(tok.name(), cmd::CMD_FVAR);
                continue;
            }
            //strvar
            known_cmds.push_back(cmd::StrVar(tok.name(), tok.docStr(), tok.defVal()));
            cmd_map.insert(tok.name(), known_cmds.size() - 1);
            cmd_types.insert(tok.name(), cmd::CMD_SVAR);
        }
    }
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
