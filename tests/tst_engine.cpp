#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "gamecfg/gameconfig.h"
#include "gamecfg/lex/lexers.h"
#include "ui/commandlistmodel.h"

/**
 * Covers the parsing engine and the bind logic the keyboard UI depends on
 * (docs/AUDIT.md #10). No GUI: everything here is a plain class or model.
 *
 * A test that pins a bug fix is named after the fix, so a future regression names
 * itself.
 */
class EngineTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // cfgLex
    void quotedTokensKeepTheirQuotes();
    void escapedQuoteStaysInToken();
    void backslashEscapeInsideQuotes();
    void commentStrippedOutsideQuotesOnly();
    void lineIsFoldedToLowerCase();
    void argumentsKeepTheirCase();
    void mixedCaseCommandNameStillTypesTheRow();
    void semicolonInsideQuotesIsNotASeparator();

    // cmdLex
    void csvRowMapsToItsColumns();
    void csvFieldWithCommaStaysOneField();
    void csvShortRowIsPadded();
    void catalogLoadsEveryRowAsACommand();

    // CommandListModel
    void appendCfgReportsAnUnreadableFile();
    void laterBindWins();
    void unbindallClearsEarlierBinds();
    void unbindClearsOneKey();
    void emptyCommandLeavesKeyUnbound();
    void setBindReplacesExistingRow();
    void setBindIsIdempotent();
    void setBindKeepsArgumentCase();
    void setBindEscapesQuotesInRow();
    void clearBindRemovesRows();

private:
    QTemporaryDir m_dir;
    int m_fileCount = 0;

    QString writeFile(const QString &name, const QString &text);
    /// Whole cfg text -> one token per token, end-of-command marked "<end>".
    QStringList tokenize(const QString &cfgText);
    QVector<cmd::Entry> parse(const QString &cfgText);
    /// One allcmds.csv row by command name; an empty name means "not found".
    static Lexers::cmdLex::Token findCmdRow(const QString &name);
};

void EngineTest::initTestCase() {
    QVERIFY2(m_dir.isValid(), qPrintable(m_dir.errorString()));
}

QString EngineTest::writeFile(const QString &name, const QString &text) {
    const QString path = m_dir.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return {};
    file.write(text.toUtf8());
    file.close();
    return path;
}

QStringList EngineTest::tokenize(const QString &cfgText) {
    QStringList tokens;
    Lexers::cfgLex lex;
    if (!lex.open(writeFile(QStringLiteral("cfg%1.cfg").arg(m_fileCount++), cfgText)))
        return tokens;
    while (lex.hasNext()) {
        const auto tok = lex.nextToken();
        tokens << (tok.isEndToken() ? QStringLiteral("<end>") : tok.string());
    }
    return tokens;
}

QVector<cmd::Entry> EngineTest::parse(const QString &cfgText) {
    GameCfg cfg;
    cfg.appendCfg(writeFile(QStringLiteral("parse%1.cfg").arg(m_fileCount++), cfgText));
    return cfg.getCmdList();
}

// --- cfgLex ---

void EngineTest::quotedTokensKeepTheirQuotes() {
    // The quotes stay on: CommandListModel::unquoted() and the Command Overview both
    // read the token as parsed (docs/legacy/ui-qml-rewrite.md D16).
    QCOMPARE(tokenize(QStringLiteral("bind \"a\" \"+moveleft\"")),
             QStringList({ "bind", "\"a\"", "\"+moveleft\"", "<end>" }));
}

void EngineTest::escapedQuoteStaysInToken() {
    // audit #6: before the fix this was 5 tokens (`"say \"` and `hello\""`). A token is
    // what the line says, backslashes included; unquoted() reads them.
    QCOMPARE(tokenize(QStringLiteral("alias t \"say \\\"hi\\\"\"")),
             QStringList({ "alias", "t", "\"say \\\"hi\\\"\"", "<end>" }));
}

void EngineTest::backslashEscapeInsideQuotes() {
    // `\\` is one escaped backslash and does not escape the following quote, so this
    // stays a single token instead of ending at the second backslash. The path keeps its
    // case: only the command name is folded.
    QCOMPARE(tokenize(QStringLiteral("alias t \"C:\\\\path\"")),
             QStringList({ "alias", "t", "\"C:\\\\path\"", "<end>" }));
}

void EngineTest::commentStrippedOutsideQuotesOnly() {
    QCOMPARE(tokenize(QStringLiteral("sv_cheats 1 // needed for cheats")),
             QStringList({ "sv_cheats", "1", "<end>" }));
    // A URL inside a quoted token is not a comment. Before the fix the line was cut at
    // `//`, left an unterminated quote and appended an "error" token to the row.
    QCOMPARE(tokenize(QStringLiteral("alias t \"echo http://host/x\"")),
             QStringList({ "alias", "t", "\"echo http://host/x\"", "<end>" }));
}

void EngineTest::lineIsFoldedToLowerCase() {
    QCOMPARE(tokenize(QStringLiteral("SV_Cheats 1")),
             QStringList({ "sv_cheats", "1", "<end>" }));
    // Leading whitespace and a trailing comment do not hide the name from the fold.
    QCOMPARE(tokenize(QStringLiteral("   Bind \"a\" \"+jump\" // go")),
             QStringList({ "bind", "\"a\"", "\"+jump\"", "<end>" }));
}

void EngineTest::argumentsKeepTheirCase() {
    // CODE_REVIEW #1: only the command name is case-insensitive. `say "Hello"` used to
    // parse as `say "hello"`, so a saved config would not survive its own reload.
    QCOMPARE(tokenize(QStringLiteral("alias t \"Say Hello\" \"Nice to meet you\"")),
             QStringList({ "alias", "t", "\"Say Hello\"", "\"Nice to meet you\"", "<end>" }));
    // A quoted first token is text, not a command name, and is left alone.
    QCOMPARE(tokenize(QStringLiteral("\"Say Hello\" x")),
             QStringList({ "\"Say Hello\"", "x", "<end>" }));
}

void EngineTest::mixedCaseCommandNameStillTypesTheRow() {
    // The lexer folds only the first name on a line, so a command after a `;` keeps its
    // case. The type lookup folds case itself, so the row is still typed correctly.
    const auto rows = parse(QStringLiteral("sv_cheats 1; Bind \"a\" \"+jump\"\n"));
    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows.at(1).type, cmd::CMD_CMD);
    QCOMPARE(rows.at(1).args.first(), QStringLiteral("Bind"));

    // And the bind scanner still reads it: `UnbindAll` after a `;` clears the earlier row.
    CommandListModel model;
    model.setEntries(parse(QStringLiteral("Bind \"a\" \"+jump\"\nsv_cheats 1; UnbindAll\n")));
    QVERIFY(!model.binds().contains(QStringLiteral("a")));
}

void EngineTest::appendCfgReportsAnUnreadableFile() {
    GameCfg cfg;
    QVERIFY(cfg.appendCfg(writeFile(QStringLiteral("ok.cfg"), QStringLiteral("bind \"a\" \"+jump\"\n"))));
    QCOMPARE(cfg.getCmdList().size(), 1);
    QVERIFY(!cfg.appendCfg(m_dir.filePath(QStringLiteral("does-not-exist.cfg"))));
}

void EngineTest::semicolonInsideQuotesIsNotASeparator() {
    // b4nny.cfg binds slot3 this way.
    QCOMPARE(tokenize(QStringLiteral("bind \"3\" \"slot3;r_drawviewmodel 1\"")),
             QStringList({ "bind", "\"3\"", "\"slot3;r_drawviewmodel 1\"", "<end>" }));
}

// --- cmdLex ---

void EngineTest::csvRowMapsToItsColumns() {
    Lexers::cmdLex lex;
    QVERIFY(lex.open(QStringLiteral(":/cfg/allcmds.csv")));

    const auto row = lex.nextToken();
    QCOMPARE(row.name(), QStringLiteral("+alt1"));
    QVERIFY(row.isCmd());
    QVERIFY(!row.hasUsage()); // input type "none"
    QCOMPARE(row.docStr(), QStringLiteral("Hold to set the alt1 key state."));
    QCOMPARE(row.inputType(), QStringLiteral("none"));
    QCOMPARE(row.platform(), QStringLiteral("universal"));
    QCOMPARE(row.context(), QStringLiteral("client controls"));
    QVERIFY(!row.hasDefault());
    QVERIFY(!row.hasRange());

    // An int-looking cvar with a range, the way GameCfg::loadAllCmds reads it.
    const auto sens = findCmdRow(QStringLiteral("sensitivity"));
    QCOMPARE(sens.name(), QStringLiteral("sensitivity"));
    QCOMPARE(sens.defVal(), QStringLiteral("3"));
    QCOMPARE(sens.minVal(), QStringLiteral("0.0001"));
    QCOMPARE(sens.maxVal(), QStringLiteral("10000000"));
    QCOMPARE(sens.inputType(), QStringLiteral("flt"));
    QVERIFY(sens.hasRange());

    // `bind` is a plain command with a two-argument signature.
    QCOMPARE(findCmdRow(QStringLiteral("bind")).inputType(), QStringLiteral("[str] [str]"));
}

Lexers::cmdLex::Token EngineTest::findCmdRow(const QString &name) {
    Lexers::cmdLex lex;
    if (!lex.open(QStringLiteral(":/cfg/allcmds.csv")))
        return Lexers::cmdLex::Token(Lexers::cmdLex::Token::TOK_EOF);
    while (lex.hasNext()) {
        const auto tok = lex.nextToken();
        if (tok.name() == name)
            return tok;
    }
    return Lexers::cmdLex::Token(Lexers::cmdLex::Token::TOK_EOF);
}

void EngineTest::csvFieldWithCommaStaysOneField() {
    // Field 7 of this row is quoted because it contains commas (res/cfg/schema.md).
    Lexers::cmdLex lex;
    QVERIFY(lex.open(QStringLiteral(":/cfg/allcmds.csv")));
    while (lex.hasNext()) {
        const auto tok = lex.nextToken();
        if (tok.name() != QLatin1String("PerfMark"))
            continue;
        QCOMPARE(tok.docStr(), QStringLiteral("inserts a telemetry marker into the stream. "
                                              "If args are provided, they will be included."));
        QCOMPARE(tok.inputType(), QStringLiteral("none"));
        QCOMPARE(tok.context(), QStringLiteral("engine"));
        return;
    }
    QFAIL("PerfMark row not found in allcmds.csv");
}

void EngineTest::csvShortRowIsPadded() {
    // Hand-edited rows may be short; the token fills the missing fields instead of
    // reading past the end (docs/CODE_REVIEW.md #2).
    Lexers::cmdLex lex;
    QVERIFY(lex.open(writeFile(QStringLiteral("short.csv"), QStringLiteral("x,cmd,,\n"))));
    const auto row = lex.nextToken();
    QCOMPARE(row.name(), QStringLiteral("x"));
    QVERIFY(row.isCmd());
    QVERIFY(row.docStr().isEmpty());
    QVERIFY(row.context().isEmpty());
    QVERIFY(!lex.hasNext());
}

void EngineTest::catalogLoadsEveryRowAsACommand() {
    // GameCfg reads ://cfg/allcmds.csv and picks the C++ subclass per input type. Rows
    // without a name are skipped, so a blank line can never become a nameless cvar.
    GameCfg cfg;
    QVERIFY(cfg.allCmds().size() > 3000);
    for (const cmd::ConVar *entry : cfg.allCmds())
        QVERIFY(!entry->get_name().isEmpty());

    // One of each type, looked up by the name the CSV carries.
    QHash<QString, const cmd::ConVar *> byName;
    for (const cmd::ConVar *entry : cfg.allCmds())
        byName.insert(entry->get_name().toLower(), entry);

    QVERIFY(byName.contains(QStringLiteral("cl_showfps")));
    QCOMPARE(byName.value(QStringLiteral("_fov"))->get_type(), cmd::CMD_BVAR);
    QCOMPARE(byName.value(QStringLiteral("cl_showfps"))->get_type(), cmd::CMD_IVAR);
    QCOMPARE(byName.value(QStringLiteral("sensitivity"))->get_type(), cmd::CMD_FVAR);
    QCOMPARE(byName.value(QStringLiteral("sensitivity"))->defaultStr(), QStringLiteral("3"));
    QCOMPARE(byName.value(QStringLiteral("bind"))->get_type(), cmd::CMD_CMD);
    QCOMPARE(byName.value(QStringLiteral("bind"))->get_usage(), QStringLiteral("[str] [str]"));
    // A str cvar is the catch-all branch: no bool/int/flt input type in the row.
    QCOMPARE(byName.value(QStringLiteral("cfgver"))->get_type(), cmd::CMD_SVAR);
    QVERIFY(byName.value(QStringLiteral("cfgver"))->defaultStr().isEmpty());
    // A cheat-tagged row keeps its flag, which is what the Build-A-Bind filter reads.
    QCOMPARE(byName.value(QStringLiteral("achievement_debug"))->get_type(), cmd::CMD_BVAR);
    QVERIFY(byName.value(QStringLiteral("achievement_debug"))->isCheat());
}

// --- CommandListModel ---

void EngineTest::laterBindWins() {
    CommandListModel model;
    model.setEntries(parse(QStringLiteral("bind \"a\" \"+moveleft\"\nbind \"a\" \"+moveright\"\n")));
    QCOMPARE(model.binds().value(QStringLiteral("a")).toString(), QStringLiteral("+moveright"));
    QCOMPARE(model.rowCount(), 2);
}

void EngineTest::unbindallClearsEarlierBinds() {
    CommandListModel model;
    model.setEntries(parse(QStringLiteral("bind \"a\" \"+moveleft\"\nunbindall\nbind \"b\" \"+jump\"\n")));
    QVERIFY(!model.binds().contains(QStringLiteral("a")));
    QCOMPARE(model.binds().value(QStringLiteral("b")).toString(), QStringLiteral("+jump"));
}

void EngineTest::unbindClearsOneKey() {
    CommandListModel model;
    model.setEntries(parse(QStringLiteral("bind \"a\" \"+moveleft\"\nbind \"b\" \"+jump\"\nunbind \"a\"\n")));
    QVERIFY(!model.binds().contains(QStringLiteral("a")));
    QCOMPARE(model.binds().value(QStringLiteral("b")).toString(), QStringLiteral("+jump"));
}

void EngineTest::emptyCommandLeavesKeyUnbound() {
    // `bind "e" ""` is how the bundled configs unbind a key.
    CommandListModel model;
    model.setEntries(parse(QStringLiteral("bind \"e\" \"\"\n")));
    QVERIFY(!model.binds().contains(QStringLiteral("e")));
}

void EngineTest::setBindReplacesExistingRow() {
    CommandListModel model;
    model.setEntries(parse(QStringLiteral("bind \"a\" \"+moveleft\"\n")));
    QVERIFY(model.setBind(QStringLiteral("a"), QStringLiteral("+duck")));
    QCOMPARE(model.binds().value(QStringLiteral("a")).toString(), QStringLiteral("+duck"));
    QCOMPARE(model.rowCount(), 1); // replaced, not appended beside the old row
}

void EngineTest::setBindIsIdempotent() {
    CommandListModel model;
    model.setEntries(parse(QStringLiteral("bind \"a\" \"+moveleft\"\n")));
    QVERIFY(!model.setBind(QStringLiteral("a"), QStringLiteral("+moveleft")));
    QVERIFY(model.setBind(QStringLiteral("a"), QStringLiteral("+duck")));
    QVERIFY(!model.setBind(QStringLiteral("a"), QStringLiteral("+duck")));
    QCOMPARE(model.rowCount(), 1);
}

void EngineTest::setBindKeepsArgumentCase() {
    // audit #16: only the command name is case-insensitive.
    CommandListModel model;
    model.setEntries(QVector<cmd::Entry>());
    QVERIFY(model.setBind(QStringLiteral("k"), QStringLiteral("Say Hello")));
    QCOMPARE(model.binds().value(QStringLiteral("k")).toString(), QStringLiteral("say Hello"));
}

void EngineTest::setBindEscapesQuotesInRow() {
    CommandListModel model;
    model.setEntries(QVector<cmd::Entry>());
    QVERIFY(model.setBind(QStringLiteral("y"), QStringLiteral("say \"hi\"")));
    // The map holds the command as the user typed it, the row holds the escaped form the
    // lexer reads back. Both directions are needed, so both are pinned here.
    QCOMPARE(model.binds().value(QStringLiteral("y")).toString(), QStringLiteral("say \"hi\""));
    QCOMPARE(model.data(model.index(0, 0), CommandListModel::LineRole).toString(),
             QStringLiteral("bind \"y\" \"say \\\"hi\\\"\""));

    // And the escaped row re-parses to the same command.
    CommandListModel reparsed;
    reparsed.setEntries(parse(model.data(model.index(0, 0), CommandListModel::LineRole).toString()));
    QCOMPARE(reparsed.binds().value(QStringLiteral("y")).toString(), QStringLiteral("say \"hi\""));
}

void EngineTest::clearBindRemovesRows() {
    CommandListModel model;
    model.setEntries(parse(QStringLiteral("bind \"a\" \"+moveleft\"\n")));
    QVERIFY(model.clearBind(QStringLiteral("a")));
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(model.binds().isEmpty());
    QVERIFY(!model.clearBind(QStringLiteral("a")));
    QVERIFY(model.clearBind(QStringLiteral("never_bound")) == false);
}

QTEST_MAIN(EngineTest)
#include "tst_engine.moc"