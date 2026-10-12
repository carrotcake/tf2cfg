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
        if (tok.name().isEmpty()) {
            continue; // EOF or malformed row
        }
        // Lookups must be case-insensitive: cmdLex keeps the CSV casing now, but
        // config lines arrive lowercased by cfgLex.
        const QString key = tok.name().toLower();

        // Owning pointers: value storage in the QVector would slice the typed
        // subclasses. Never freed - GameCfg lives for the whole app run.
        if (tok.isCmd()) {
            known_cmds.push_back(new cmd::ConCmd(tok.name(), tok.docStr(), tok.inputType(),
                                                 tok.flagsVal(), tok.platform(), tok.context()));
            cmd_types.insert(key, cmd::CMD_CMD);
        } else if (tok.inputType() == QLatin1String("bool")) {
            known_cmds.push_back(new cmd::BoolVar(tok.name(), tok.docStr(),
                                                  tok.defVal() == QLatin1String("1"),
                                                  tok.flagsVal(), tok.platform(), tok.context()));
            cmd_types.insert(key, cmd::CMD_BVAR);
        } else if (tok.inputType() == QLatin1String("int")) {
            bool ok = false;
            auto def = tok.defVal().toLongLong(&ok);
            if (!ok) {
                def = 0;
            }
            if (tok.hasRange()) {
                known_cmds.push_back(new cmd::IntVar(tok.name(), tok.docStr(), def,
                                                     tok.minVal().toLongLong(), tok.maxVal().toLongLong(),
                                                     tok.flagsVal(), tok.platform(), tok.context()));
            } else {
                known_cmds.push_back(new cmd::IntVar(tok.name(), tok.docStr(), def,
                                                     tok.flagsVal(), tok.platform(), tok.context()));
            }
            cmd_types.insert(key, cmd::CMD_IVAR);
        } else if (tok.inputType() == QLatin1String("flt")) {
            bool ok = false;
            double def = tok.defVal().toDouble(&ok);
            if (!ok) {
                def = 0.0;
            }
            if (tok.hasRange()) {
                known_cmds.push_back(new cmd::FltVar(tok.name(), tok.docStr(), def,
                                                     tok.minVal().toDouble(), tok.maxVal().toDouble(),
                                                     tok.flagsVal(), tok.platform(), tok.context()));
            } else {
                known_cmds.push_back(new cmd::FltVar(tok.name(), tok.docStr(), def,
                                                     tok.flagsVal(), tok.platform(), tok.context()));
            }
            cmd_types.insert(key, cmd::CMD_FVAR);
        } else {
            // str: includes cvars with an empty default, which v4 silently dropped.
            known_cmds.push_back(new cmd::StrVar(tok.name(), tok.docStr(), tok.defVal(),
                                                 tok.flagsVal(), tok.platform(), tok.context()));
            cmd_types.insert(key, cmd::CMD_SVAR);
        }
        cmd_map.insert(key, known_cmds.size() - 1);
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
