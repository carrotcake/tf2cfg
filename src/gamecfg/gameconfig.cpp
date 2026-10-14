#include "gameconfig.h"
#include "lex/lexers.h"

#include <QDebug>
#include <QLoggingCategory>

#include <algorithm>

Q_LOGGING_CATEGORY(lcCfg, "tf2cfg.config")

GameCfg::GameCfg() {
    loadAllCmds();
    loadArgBounds();
}

GameCfg::GameCfg(const QString &filename)
    : GameCfg() {
    appendCfg(filename);
}

bool GameCfg::appendCfg(const QString &filename) {
    Lexers::cfgLex lex;
    cmd::Entry cmd;

    if (!lex.open(filename)) {
        qWarning("GameCfg: cannot read %s", qPrintable(filename));
        return false;
    }

    while (lex.hasNext()) {
        // The lexer tokenizes line by line, so the cursor's line index is the file line
        // the command started on. Recorded per row: the surgical save regenerates exactly
        // these lines (docs/legacy/ui-qml-rewrite.md 10.1).
        cmd.sourceLine = lex.currentLine();
        const auto tok = lex.nextToken();
        if (cmd.args.empty()) {
            cmd.type = cmdToType(tok.string());
        }
        if (cmd.args.size() == 1 && !cmd.args.front().compare("alias")) {
            // The alias NAME is what later rows look up; tagged in the per-parse map
            // (see alias_types in gameconfig.h).
            alias_types[tok.string().toLower()] = cmd::CMD_ALIAS;
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
    return true;
}

void GameCfg::resetParsed() {
    cmd_list.clear();
    alias_types.clear();
}

inline cmd::CmdType
GameCfg::cmdToType(const QString &cmd) const {
    // Case-insensitive: cfgLex folds the command name, but a config may still spell it
    // `Bind`, and everything after a `;` on one line keeps its own case.
    const QString key = cmd.toLower();
    // An alias name the current parse defined wins: a later `bind "k" "myalias"` row
    // must type as alias even when the name shadows a catalog entry. Catalog types
    // survive every resetParsed() - only alias_types is per-parse.
    auto alias = alias_types.constFind(key);
    if (alias != alias_types.cend())
        return alias.value();
    auto it = cmd_types.constFind(key);
    return it == cmd_types.cend() ? cmd::CMD_UNKWN : it.value();
}

void GameCfg::loadAllCmds() {
    Lexers::cmdLex lex;
    if (!lex.open("://cfg/allcmds.csv")) {
        qWarning("GameCfg: cannot read the command catalog from the resource bundle");
        return;
    }
    while (lex.hasNext()) {
        const auto tok = lex.nextToken();
        if (tok.name().isEmpty()) {
            continue; // EOF or malformed row
        }
        // Lookups must be case-insensitive: cmdLex keeps the CSV casing now, but
        // config lines arrive lowercased by cfgLex.
        const QString key = tok.name().toLower();

        // Owning pointers: value storage in the QVector would slice the typed
        // subclasses. Freed in ~GameCfg (audit #1).
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
            } else if (tok.hasMin() || tok.hasMax()) {
                // One-sided: the engine clamps each side independently, so keep the
                // present side and treat the absent one as 0 (its value is unused).
                known_cmds.push_back(new cmd::IntVar(tok.name(), tok.docStr(), def,
                                                     tok.minVal().toLongLong(), tok.maxVal().toLongLong(),
                                                     tok.hasMin(), tok.hasMax(),
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
            } else if (tok.hasMin() || tok.hasMax()) {
                known_cmds.push_back(new cmd::FltVar(tok.name(), tok.docStr(), def,
                                                     tok.minVal().toDouble(), tok.maxVal().toDouble(),
                                                     tok.hasMin(), tok.hasMax(),
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

/**
 * Per-argument authoring bounds from the companion catalog file
 * (docs/DESIGN-arg-bounds.md §4). Row layout: command, argIndex, type (int/flt),
 * min, max - empty min or max means that side is unbounded, the way the engine's
 * independent clamps work. Every invalid row is warned about and skipped; a missing
 * file is normal and leaves every command unbounded.
 *
 * The catalog is the shape authority: bounds only attach to commands allcmds.csv
 * knows as cmd rows, and an arg index must land inside the command's usage
 * signature. cmdLex::Token::field() maps the 5 columns onto fields 0-4.
 */
void GameCfg::loadArgBounds(const QString &path) {
    // command -> arg index -> spec, so a command's specs can be validated as a set.
    QHash<QString, QHash<int, cmd::ArgSpec>> collected;

    Lexers::cmdLex lex;
    if (!lex.open(path)) {
        // Optional file by design; only a debug note, never an error.
        qCDebug(lcCfg, "no arg bounds at %s - commands stay unbounded", qPrintable(path));
        return;
    }
    while (lex.hasNext()) {
        const auto tok = lex.nextToken();
        if (tok.name().isEmpty()) {
            continue; // EOF or malformed row
        }
        const QString key = tok.name().toLower();
        bool okIndex = false;
        const int index = tok.field(1).toInt(&okIndex);
        const QString type = tok.field(2);
        if (!okIndex || index < 0 || (type != QLatin1String("int")
                                      && type != QLatin1String("flt"))) {
            qWarning("argbounds: skipping malformed row for %s", qPrintable(key));
            continue;
        }
        bool okMin = false;
        bool okMax = false;
        const double min = tok.field(3).toDouble(&okMin);
        const double max = tok.field(4).toDouble(&okMax);
        const bool hasMin = !tok.field(3).isEmpty() && okMin;
        const bool hasMax = !tok.field(4).isEmpty() && okMax;
        if ((hasMin && !okMin) || (hasMax && !okMax) || (!hasMin && !hasMax)) {
            qWarning("argbounds: skipping unparseable bounds for %s arg %d",
                     qPrintable(key), index);
            continue;
        }
        if (hasMin && hasMax && min > max) {
            qWarning("argbounds: min > max for %s arg %d", qPrintable(key), index);
            continue;
        }
        if (collected.value(key).contains(index)) {
            qWarning("argbounds: duplicate entry for %s arg %d", qPrintable(key), index);
            continue;
        }
        cmd::ArgSpec spec;
        spec.index = index;
        spec.isFloat = type == QLatin1String("flt");
        spec.hasMin = hasMin;
        spec.min = min;
        spec.hasMax = hasMax;
        spec.max = max;
        collected[key][index] = spec;
    }

    for (auto it = collected.constBegin(); it != collected.constEnd(); ++it) {
        const auto ci = cmd_map.constFind(it.key());
        if (ci == cmd_map.cend() || cmd_types.value(it.key()) != cmd::CMD_CMD) {
            qWarning("argbounds: %s is not a known command", qPrintable(it.key()));
            continue;
        }
        // The usage signature is the shape authority: "[str] [flt] ..." tokens, one
        // per argument; a command with none accepts no bounds at all.
        const QStringList tokens =
            known_cmds.at(ci.value())->get_usage().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        const int argc = std::count_if(tokens.cbegin(), tokens.cend(), [](const QString &t) {
            return t.startsWith(QLatin1Char('[')) || t.startsWith(QLatin1Char('<'));
        });
        if (argc == 0) {
            qWarning("argbounds: %s takes no documented arguments", qPrintable(it.key()));
            continue;
        }
        QVector<cmd::ArgSpec> specs;
        specs.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            if (it.value().contains(i))
                specs.append(it.value().value(i));
        }
        if (specs.isEmpty())
            continue; // every row for this command failed validation
        std::sort(specs.begin(), specs.end(),
                  [](const cmd::ArgSpec &a, const cmd::ArgSpec &b) { return a.index < b.index; });
        static_cast<cmd::ConCmd *>(known_cmds.at(ci.value()))->setArgSpecs(specs);
    }
}

// Parity P12/D7: the widget build dumped the parsed list on load, so it still happens.
// It goes through qInfo rather than raw printf: no std::string per token, and the
// category is what a future `QT_LOGGING_RULES` can silence (review finding 17).
// The line text is unchanged; the channel is not - Qt logs to stderr.
void GameCfg::printCfg() const {
    for (const cmd::Entry &cmd : cmd_list) {
        QString line;
        for (const QString &str : cmd.args) {
            line += str + ' ';
        }
        line += QStringLiteral(" (type: %1) ").arg(static_cast<int>(cmd.type));
        qCDebug(lcCfg).noquote() << line;
    }
}
QVector<cmd::Entry> const &GameCfg::getCmdList() const {
    return cmd_list;
}

GameCfg::~GameCfg() {
    qDeleteAll(known_cmds);
}
