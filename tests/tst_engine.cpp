#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QtTest>

#include "gamecfg/gameconfig.h"
#include "gamecfg/lex/lexers.h"
#include "ui/commandfiltermodel.h"
#include "ui/commandlistmodel.h"
#include "ui/keytable.h"
#include "ui/configcontroller.h"
#include "ui/consolecommandmodel.h"

/**
 * A hand-built catalog, so a filter or sort test does not depend on allcmds.csv drifting.
 * Owns its entries; the model only borrows them, exactly as GameCfg does.
 */
struct CatalogFixture {
    std::vector<std::unique_ptr<cmd::ConVar>> owned;

    QVector<cmd::ConVar *> make() {
        auto add = [this](cmd::ConVar *entry) {
            owned.emplace_back(entry);
            return entry;
        };
        add(new cmd::ConCmd(QStringLiteral("bind"), QStringLiteral("Bind a key."),
                             QStringLiteral("[str] [str]"), QStringLiteral("norecord"),
                             QStringLiteral("universal"), QStringLiteral("engine")));
        add(new cmd::ConCmd(QStringLiteral("+attack"), QStringLiteral("Fire weapon."),
                             QStringLiteral("none"), QStringLiteral("client"),
                             QStringLiteral("universal"), QStringLiteral("client controls")));
        add(new cmd::ConCmd(QStringLiteral("+jump"), QStringLiteral("Jump."),
                             QStringLiteral("none"), QStringLiteral("cheat client"),
                             QStringLiteral("universal"), QStringLiteral("client controls")));
        add(new cmd::ConCmd(QStringLiteral("zapper"), QStringLiteral("Linux only."),
                             QStringLiteral("none"), QStringLiteral("client"),
                             QStringLiteral("linux"), QStringLiteral("client render")));
        add(new cmd::FltVar(QStringLiteral("sensitivity"), QStringLiteral("Mouse sensitivity."),
                            3.0, 0.0001, 10000000.0, QStringLiteral("client archive"),
                            QStringLiteral("universal"), QStringLiteral("client settings")));
        add(new cmd::BoolVar(QStringLiteral("_fov"), QStringLiteral("Automates fov."), false,
                             QStringLiteral("client"), QStringLiteral("universal"),
                             QStringLiteral("client settings")));

        QVector<cmd::ConVar *> out;
        for (const auto &entry : owned)
            out << entry.get();
        return out;
    }
};

/**
 * Covers the parsing engine and the bind logic the keyboard UI depends on
 * (docs/legacy/AUDIT.md #10). No GUI: everything here is a plain class or model.
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
    void catalogRowsHaveTenFields();
    void catalogTypeFieldIsEmptyOrCommand();
    void catalogRowsAddedFromTheLiveDump();
    void catalogRowsCheckedInGame();
    void catalogRowsFixedFromTheInGameRun();

    // Per-argument + one-sided bounds (docs/DESIGN-arg-bounds.md)
    void oneSidedCvarBoundsKeepTheirSide();
    void argBoundsAttachFromCompanionFile();
    void argBoundsTolerateAMissingFile();
    void argBoundsFromTheInGameRun();
    void signatureParsesIntoEditorArguments();
    void argsSpecRoleCarriesPerArgumentBounds();

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
    void setBindDropsLaterRowsForKey();
    void bindingKindReportsImportedToggle();
    void setBindToggleWritesToggleRow();

    // ConsoleCommandModel - the Build-A-Bind browse list
    void catalogSortsByContextThenName();
    void contextAndPlatformFiltersNarrowRows();
    void cheatRowsAreHiddenUnlessAsked();
    void filtersSurviveBeingSwitchedBackAndForth();
    void suggestOnlyOffersVisibleNames();
    void suggestNeedsSomeText();
    void findCommandIgnoresCaseAndFilters();
    void findCommandCarriesTheFullRecord();
    void commandAtCarriesTheArgumentMetadata();

    // ConfigController: the surgical save (docs/legacy/ui-qml-rewrite.md 10.1)
    void saveRewritesOnlyOwnedLines();
    void savePreservesCrlfAndMissingFinalNewline();
    void saveDropsDeletedBinds();
    void saveAppendsSessionBindsAfterTheOwnedBlock();
    void saveIsIdempotent();
    void saveWritesABakBesideTheFile();
    void failedBackupWriteKeepsTheOldRestorePoint();

    // Key table (deviation D9: the QML singleton is now the C++ KeyTable singleton of the
    // tf2cfg module)
    void keyTableParityInvariants();    void saveRefusesAResourcePath();
    void saveRecreatesAVanishedFile();
    void saveDetectsAnExternallyModifiedFile();

    // Config Editor (docs/DESIGN-config-editor.md, chunk A)
    void parseCommandLineTokenizesLikeTheLexer();
    void setRowTextReclassifiesAndOwnsTheRow();
    void deleteRowDropsItsLineOnSave();
    void insertRowAfterLandsAfterTheAnchorLine();
    void insertRowAfterChainsSessionAnchors();
    void warningsFlagDeadBindsAndOverriddenCvars();
    void filterModelKindsAndText();

    // Aliases + chains (docs/DESIGN-config-editor.md §5, chunk C)
    void setAliasCreatesARoundTrippingRow();
    void setAliasEditsInPlaceAndDropsDuplicates();
    void setAliasEmptyBodyRemoves();
    void setAliasAnchorsAfterTheLastAliasLine();
    void splitSegmentsIsQuoteAware();
    void quoteUnquoteRoundTrips();

    // Comments (docs/DESIGN-config-editor.md §6, chunk D)
    void commentRowsParseAndRoundTrip();
    void trailingCommentEditsThroughTheRawField();
    void compoundInsertStaysOneLine();
    void compoundTailSurvivesAnEdit();

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
    // No QIODevice::Text: the save tests assert exact line endings, so the bytes on
    // disk must be exactly the bytes handed over (\n stays \n on Windows too).
    if (!file.open(QIODevice::WriteOnly))
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
    // reading past the end (docs/legacy/CODE_REVIEW.md #2).
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

void EngineTest::catalogRowsHaveTenFields() {
    // Catalog ingestion note: allcmds.csv is machine-generated and every shipped row
    // must carry exactly the 10 columns documented in res/cfg/schema.md. This pins
    // that constraint so a malformed row cannot silently be padded or truncated
    // by cmdLex's tolerant scanner (docs/AUDIT.md #5).
    QFile file(QStringLiteral(":/cfg/allcmds.csv"));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream in(&file);
    int rows = 0;
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.isEmpty())
            continue;
        int commas = 0;
        bool inQuotes = false;
        for (int i = 0; i < line.size(); ++i) {
            const QChar c = line.at(i);
            if (inQuotes) {
                if (c == QLatin1Char('"')) {
                    if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"'))
                        ++i;
                    else
                        inQuotes = false;
                }
            } else {
                if (c == QLatin1Char('"'))
                    inQuotes = true;
                else if (c == QLatin1Char(','))
                    ++commas;
            }
        }
        QVERIFY2(!inQuotes, qPrintable(QStringLiteral("unclosed quote in catalog row: ") + line));
        QCOMPARE(commas + 1, 10);
        ++rows;
    }
    QCOMPARE(rows, 3845);
}

void EngineTest::catalogTypeFieldIsEmptyOrCommand() {
    // res/cfg/schema.md defines field 2 as cmd / ncmd / empty (= a console variable), but
    // cmdLex::Token::isCmd() asks only whether the field is non-empty. The 58 rows that
    // said "cvar" therefore loaded as commands - they sat under the editor's Commands
    // filter, showed "bool"/"int" where a usage string belongs, and got a command editor
    // in the guided pane. The live dump reports a value for every one of them, not `cmd`
    // (tools/catalog_diff.py).
    QFile file(QStringLiteral(":/cfg/allcmds.csv"));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream in(&file);
    QHash<QString, int> census;
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.isEmpty())
            continue;
        // A quoted name can carry a comma; the type field itself never does.
        const int firstComma = line.startsWith(QLatin1Char('"'))
            ? line.indexOf(QStringLiteral("\",")) + 1
            : line.indexOf(QLatin1Char(','));
        QVERIFY2(firstComma > 0, qPrintable(line));
        const QString type =
            line.mid(firstComma + 1, line.indexOf(QLatin1Char(','), firstComma + 1) - firstComma - 1);
        QVERIFY2(type.isEmpty() || type == QLatin1String("cmd") || type == QLatin1String("ncmd"),
                 qPrintable(QStringLiteral("bad type field \"%1\" in row: %2").arg(type, line)));
        census[type.isEmpty() ? QStringLiteral("variable") : type] += 1;
    }
    QCOMPARE(census.value(QStringLiteral("variable")), 2802);
    QCOMPARE(census.value(QStringLiteral("cmd")), 1009);
    QCOMPARE(census.value(QStringLiteral("ncmd")), 34);

    // The app-level effect: two of the retyped rows are variables now, each in the
    // subclass its input-type column asks for. (Two others, binds_per_command and
    // cl_playerspraydisable, were retyped the same way and then removed - the 2026-10-15
    // second run found the client has neither.)
    GameCfg cfg;
    QHash<QString, const cmd::ConVar *> byName;
    for (const cmd::ConVar *entry : cfg.allCmds())
        byName.insert(entry->get_name().toLower(), entry);
    QCOMPARE(byName.value(QStringLiteral("gl_blitmode"))->get_type(), cmd::CMD_IVAR);
    QCOMPARE(byName.value(QStringLiteral("gl_batch_tex_creates"))->get_type(), cmd::CMD_BVAR);
}

void EngineTest::catalogRowsAddedFromTheLiveDump() {
    // The 2026-10-15 Windows dump (data/tf2-cvarlist-10-26-gamedump-w64.txt) carries 56 names
    // the catalog lacked - cvars and commands added to the client since it was generated. The
    // dump itself gives type, default, flags and help text; the input type and the context
    // column come from res/cfg/schema.md's rules, and the signature of a command whose usage
    // line nobody has seen stays `none` until an in-game run says otherwise.
    GameCfg cfg;
    QHash<QString, const cmd::ConVar *> byName;
    for (const cmd::ConVar *entry : cfg.allCmds())
        byName.insert(entry->get_name().toLower(), entry);

    // A cheat-tagged cvar that arrived with the dump, and its neighbour without the tag.
    QCOMPARE(byName.value(QStringLiteral("fog_radial"))->get_type(), cmd::CMD_IVAR);
    QVERIFY(byName.value(QStringLiteral("fog_radial"))->isCheat());
    QVERIFY(!byName.value(QStringLiteral("r_lightmap_bicubic"))->isCheat());

    // Scale factors and rates are floats although the client prints them without a decimal
    // point, the same call the catalog already makes for `sensitivity`.
    QCOMPARE(byName.value(QStringLiteral("vgui_ui_scale_factor"))->get_type(), cmd::CMD_FVAR);
    QCOMPARE(byName.value(QStringLiteral("snd_rear_speaker_scale"))->get_type(), cmd::CMD_FVAR);
    QCOMPARE(byName.value(QStringLiteral("tf_armory_page_skip"))->get_type(), cmd::CMD_IVAR);
    QCOMPARE(byName.value(QStringLiteral("tf_armory_page_skip"))->defaultStr(), QStringLiteral("10"));

    // A new command, and a cvar the client still spells with its own typo.
    QCOMPARE(byName.value(QStringLiteral("print_froggy"))->get_type(), cmd::CMD_CMD);
    QCOMPARE(byName.value(QStringLiteral("print_froggy"))->get_usage(), QStringLiteral("none"));
    QVERIFY(byName.contains(QStringLiteral("sv_allow_server_adverisement_to_master_server")));

    // Description text the dump corrected: four typos and one rewritten sentence.
    QVERIFY(!byName.value(QStringLiteral("set_party_contract_progress_enabled"))
                 ->get_docs()
                 .contains(QStringLiteral("memebers")));
    QVERIFY(byName.value(QStringLiteral("sv_vote_quorum_ratio"))->get_docs().contains(QStringLiteral("Min 0.1")));
    QVERIFY(byName.value(QStringLiteral("sv_master_share_game_socket"))
                ->get_docs()
                .contains(QStringLiteral("server browser traffic")));

    // Two descriptions deliberately keep the catalog's wording: `disguise` carries the usage
    // string the in-game run read off the console, and `sv_pure`'s own help text in the client
    // is another command's ("Show user data.") - both settled in-game on 2026-10-15.
    QVERIFY(byName.value(QStringLiteral("disguise"))->get_docs().contains(QStringLiteral("Usage: disguise")));
}

void EngineTest::catalogRowsCheckedInGame() {
    // data/checks.txt - the second 2026-10-15 run, on a listen server with sv_cheats 0 - asked
    // the client what `cvarlist` could not answer (docs/in-game-checklist.md §4). Three answers:
    //
    // 1. 35 names printed "Unknown command" and lost their rows. But r_spewleaf answered with a
    //    value although cvarlist never lists it, so an absent name is a candidate, never proof.
    for (const auto &gone : {QStringLiteral("bench_upload"), QStringLiteral("binds_per_command"),
                             QStringLiteral("cl_playerspraydisable"),
                             QStringLiteral("dxa_nullrefresh_capslock"),
                             QStringLiteral("hud_magnetism"), QStringLiteral("sdl_double_click_time"),
                             QStringLiteral("snd_lockpartial"),
                             QStringLiteral("video_quicktime_encode_gamma"),
                             QStringLiteral("vr_track_reinit"),
                             QStringLiteral("windows_speaker_config")})
        QVERIFY2(findCmdRow(gone).isEndToken(),
                 qPrintable(QStringLiteral("row still in the catalog: ") + gone));
    QVERIFY(!findCmdRow(QStringLiteral("r_spewleaf")).isEndToken());

    // 2. The 20 commands added from the dump all ran bare without printing a usage line, so
    //    `none` stands as their signature. The twelve snd_* speaker commands answer with their
    //    setting ("snd_front_headphone_position = 90.000000"), which is why the dump calls them
    //    commands rather than variables.
    QCOMPARE(findCmdRow(QStringLiteral("snd_front_headphone_position")).inputType(), QStringLiteral("none"));
    QCOMPARE(findCmdRow(QStringLiteral("print_froggy")).inputType(), QStringLiteral("none"));

    // 3. 13 rows lost their `cheat` tag: the wiki carried it, but both live dumps agree with the
    //    run, where a bare set with sv_cheats 0 was never refused.
    GameCfg cfg;
    QHash<QString, const cmd::ConVar *> byName;
    for (const cmd::ConVar *entry : cfg.allCmds())
        byName.insert(entry->get_name().toLower(), entry);
    QVERIFY(!byName.value(QStringLiteral("r_shadows"))->isCheat());
    QVERIFY(!byName.value(QStringLiteral("sv_memlimit"))->isCheat());
    QVERIFY(!byName.value(QStringLiteral("tf_powerup_mode_imbalance_delta"))->isCheat());
    QCOMPARE(byName.value(QStringLiteral("r_drawropes"))->get_flags(), QStringLiteral("client"));
    // A cheat tag the dump did report stays: fog_radial arrived with one.
    QVERIFY(byName.value(QStringLiteral("fog_radial"))->isCheat());
}

void EngineTest::catalogRowsFixedFromTheInGameRun() {
    // Four rows the in-game run of 2026-10-15 changed (docs/in-game-checklist.md §1).
    // The console prints a command's real usage when it is typed bare, and that beat the
    // dump the catalog was generated from.
    const auto bench = findCmdRow(QStringLiteral("benchframe"));
    QCOMPARE(bench.name(), QStringLiteral("benchframe"));
    QCOMPARE(bench.inputType(), QStringLiteral("[str] [int] [str]"));

    // `bug` opens the reporter with or without an argument, so it takes none.
    const auto bug = findCmdRow(QStringLiteral("bug"));
    QCOMPARE(bug.inputType(), QStringLiteral("none"));

    // The dump's "Show user data." was another command's text, not sv_pure's rule list.
    const auto svPure = findCmdRow(QStringLiteral("sv_pure"));
    QCOMPARE(svPure.name(), QStringLiteral("sv_pure"));
    QVERIFY(svPure.docStr().contains(QStringLiteral("pure_server_full.txt")));

    // `grab_window` answers "Unknown command" in the live client, so the row is gone
    // rather than kept as a pickable entry that cannot work.
    QVERIFY(findCmdRow(QStringLiteral("grab_window")).isEndToken());
}

// --- Per-argument + one-sided bounds (docs/DESIGN-arg-bounds.md) ---

void EngineTest::oneSidedCvarBoundsKeepTheirSide() {
    // The engine clamps each side independently; a lone min (35 catalog rows carry
    // exactly that) must survive the loader instead of being dropped as "no range".
    const cmd::IntVar ivar(QStringLiteral("t"), QStringLiteral("d"), 1, -5, 0,
                           true, false, QStringLiteral("client"), QStringLiteral("universal"),
                           QStringLiteral("client settings"));
    QVERIFY(ivar.hasMinBound());
    QVERIFY(!ivar.hasMaxBound());
    QVERIFY(ivar.hasRange());
    QCOMPARE(ivar.minVal(), -5.0);

    const cmd::FltVar fvar(QStringLiteral("t"), QStringLiteral("d"), 1.0, 0.0, 2.5,
                           false, true, QStringLiteral("client"), QStringLiteral("universal"),
                           QStringLiteral("client settings"));
    QVERIFY(!fvar.hasMinBound());
    QVERIFY(fvar.hasMaxBound());
    QCOMPARE(fvar.maxVal(), 2.5);

    // And the catalog's own min-only rows now keep their side.
    const auto row = findCmdRow(QStringLiteral("mp_maxrounds"));
    QVERIFY(row.hasMin());
    QVERIFY(!row.hasMax());
}

void EngineTest::argBoundsAttachFromCompanionFile() {
    GameCfg cfg; // the default ctor already loaded the bundled argbounds.csv

    // A temp companion file mixing valid rows and every rejection path:
    // addcond [int] <int>, drawline six [flt]s, mp_maxrounds is a cvar.
    const QStringList lines = {
        QStringLiteral("addcond,0,int,0,"),        // min only, int
        QStringLiteral("drawline,2,flt,,-1"),      // max only, flt
        QStringLiteral("nosuchcmd,0,int,0,1"),     // unknown command: skipped
        QStringLiteral("addcond,5,int,0,1"),       // arg index outside the signature
        QStringLiteral("mp_maxrounds,0,int,0,1"),  // not a cmd row: skipped
        QStringLiteral("addcond,1,flt,10,5"),      // min > max: skipped
        QStringLiteral("drawline,3,flt,-5,5"),     // second valid row for sorting
    };
    const QString path = writeFile(QStringLiteral("argbounds-test.csv"), lines.join('\n') + '\n');
    cfg.loadArgBounds(path);

    const auto findCmd = [&cfg](const QString &name) -> cmd::ConCmd * {
        for (cmd::ConVar *cv : cfg.allCmds())
            if (cv->get_name() == name && cv->get_type() == cmd::CMD_CMD)
                return static_cast<cmd::ConCmd *>(cv);
        return nullptr;
    };

    cmd::ConCmd *addcond = findCmd(QStringLiteral("addcond"));
    QVERIFY(addcond);
    QCOMPARE(addcond->argSpecs().size(), 1);
    QCOMPARE(addcond->argSpecs().at(0).index, 0);
    QVERIFY(!addcond->argSpecs().at(0).isFloat);
    QVERIFY(addcond->argSpecs().at(0).hasMin);
    QCOMPARE(addcond->argSpecs().at(0).min, 0.0);
    QVERIFY(!addcond->argSpecs().at(0).hasMax);

    cmd::ConCmd *drawline = findCmd(QStringLiteral("drawline"));
    QVERIFY(drawline);
    QCOMPARE(drawline->argSpecs().size(), 2);
    QCOMPARE(drawline->argSpecs().at(0).index, 2); // sorted by index
    QVERIFY(drawline->argSpecs().at(0).isFloat);
    QVERIFY(!drawline->argSpecs().at(0).hasMin);
    QVERIFY(drawline->argSpecs().at(0).hasMax);
    QCOMPARE(drawline->argSpecs().at(0).max, -1.0);
    QCOMPARE(drawline->argSpecs().at(1).index, 3);
    QCOMPARE(drawline->argSpecs().at(1).min, -5.0);
    QCOMPARE(drawline->argSpecs().at(1).max, 5.0);

    // The bundled resource file already carries the user-verified seed row.
    cmd::ConCmd *lip = findCmd(QStringLiteral("load_itempreset"));
    QVERIFY(lip);
    QCOMPARE(lip->argSpecs().size(), 1);
    QCOMPARE(lip->argSpecs().at(0).min, 0.0);
    QCOMPARE(lip->argSpecs().at(0).max, 3.0);
}

void EngineTest::argBoundsTolerateAMissingFile() {
    GameCfg cfg;
    // A missing companion file is normal: no crash, existing specs untouched.
    cfg.loadArgBounds(m_dir.filePath(QStringLiteral("no-such-argbounds.csv")));
    const auto findLip = [&cfg]() -> cmd::ConCmd * {
        for (cmd::ConVar *cv : cfg.allCmds())
            if (cv->get_name() == QStringLiteral("load_itempreset"))
                return static_cast<cmd::ConCmd *>(cv);
        return nullptr;
    };
    cmd::ConCmd *lip = findLip();
    QVERIFY(lip);
    QCOMPARE(lip->argSpecs().size(), 1); // the resource-loaded spec survives
}

void EngineTest::argBoundsFromTheInGameRun() {
    GameCfg cfg; // the default ctor already loaded the bundled argbounds.csv

    const auto findCmd = [&cfg](const QString &name) -> cmd::ConCmd * {
        for (cmd::ConVar *cv : cfg.allCmds())
            if (cv->get_name() == name && cv->get_type() == cmd::CMD_CMD)
                return static_cast<cmd::ConCmd *>(cv);
        return nullptr;
    };

    // Every row below was verified in-game on 2026-10-15 and applied that day
    // (docs/argbounds-verification.md). The run's evidence is the console's own usage
    // line, which is why these sit at "highest" confidence.
    struct Expect {
        const char *cmd;
        int index;
        bool isFloat;
        bool hasMin;
        double min;
        bool hasMax;
        double max;
    };
    const QVector<Expect> expected = {
        // sv_pure: the bare command prints -1/0/1/2 and rejects 3 (data/sv_pure.txt).
        { "sv_pure", 0, false, true, -1.0, true, 2.0 },
        // fs_warning_level: the bare command enumerates FILESYSTEM_WARNING_* 0..6.
        { "fs_warning_level", 0, false, true, 0.0, true, 6.0 },
        // maxplayers: 101 is the ceiling; 0 is accepted but the server never starts.
        { "maxplayers", 0, false, true, 1.0, true, 101.0 },
        // mat_setvideomode's windowed argument: 2 and up wrap to 1.
        { "mat_setvideomode", 2, false, true, 0.0, true, 1.0 },
        { "mat_enable_vrmode", 0, false, true, 0.0, true, 1.0 },
        // cc_bot_selectweapon's slot: 0..2; higher values are silently ignored.
        { "cc_bot_selectweapon", 1, false, true, 0.0, true, 2.0 },
        // Floors only: playvol wraps -1 to 0, demo_timescale freezes at 0.
        { "playvol", 1, true, true, 0.0, false, 0.0 },
        { "demo_timescale", 0, true, true, 0.0, false, 0.0 },
    };

    for (const Expect &e : expected) {
        const QString name = QString::fromLatin1(e.cmd);
        cmd::ConCmd *cmd = findCmd(name);
        QVERIFY2(cmd, qPrintable(name));
        QCOMPARE(cmd->argSpecs().size(), 1);
        const cmd::ArgSpec &spec = cmd->argSpecs().at(0);
        QCOMPARE(spec.index, e.index);
        QCOMPARE(spec.isFloat, e.isFloat);
        QCOMPARE(spec.hasMin, e.hasMin);
        QCOMPARE(spec.hasMax, e.hasMax);
        if (e.hasMin)
            QCOMPARE(spec.min, e.min);
        if (e.hasMax)
            QCOMPARE(spec.max, e.max);
    }
}

void EngineTest::signatureParsesIntoEditorArguments() {
    // The one parse of allcmds field 8 (cmd::parseSignature): the Build-A-Bind and guided
    // editors turn it into one editor per argument, and loadArgBounds validates its row
    // indexes against the same shape, so the two cannot disagree about how many there are.
    const QVector<cmd::SignatureArg> two = cmd::parseSignature(QStringLiteral("[str] <flt>"));
    QCOMPARE(two.size(), 2);
    QCOMPARE(two.at(0).kind, QStringLiteral("str"));
    QVERIFY(!two.at(0).optional);
    QCOMPARE(two.at(1).kind, QStringLiteral("flt"));
    QVERIFY(two.at(1).optional);

    // No arguments: the catalog's "none", an empty field, and stray whitespace between tokens.
    QVERIFY(cmd::parseSignature(QStringLiteral("none")).isEmpty());
    QVERIFY(cmd::parseSignature(QString()).isEmpty());
    QCOMPARE(cmd::parseSignature(QStringLiteral("  [int]   <int> ")).size(), 2);

    // A cvar's "signature" is its value type: bare, mandatory, passed through as written.
    const QVector<cmd::SignatureArg> bare = cmd::parseSignature(QStringLiteral("bool"));
    QCOMPARE(bare.size(), 1);
    QCOMPARE(bare.at(0).kind, QStringLiteral("bool"));
    QVERIFY(!bare.at(0).optional);

    // Only a clean lower-case kind loses its brackets; an oddity reaches the editor verbatim
    // instead of being silently rewritten (the catalog has none, hand-edited rows might).
    QCOMPARE(cmd::parseSignature(QStringLiteral("[int] [a|b]")).at(1).kind, QStringLiteral("[a|b]"));

    // And the shape QML gets: one {kind, optional} map per argument.
    ConfigController controller;
    const QVariantList forQml = controller.parseSignature(QStringLiteral("[str] <flt>"));
    QCOMPARE(forQml.size(), 2);
    QCOMPARE(forQml.at(0).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("str"));
    QCOMPARE(forQml.at(0).toMap().value(QStringLiteral("optional")).toBool(), false);
    QCOMPARE(forQml.at(1).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("flt"));
    QCOMPARE(forQml.at(1).toMap().value(QStringLiteral("optional")).toBool(), true);
    QVERIFY(controller.parseSignature(QStringLiteral("none")).isEmpty());
}

void EngineTest::argsSpecRoleCarriesPerArgumentBounds() {
    CatalogFixture fixture;
    QVector<cmd::ConVar *> cmds = fixture.make();

    // Give `bind` ([str] [str]) bounds on its first argument only, to prove the role
    // reports per-argument, one-sided data instead of one range per row.
    for (cmd::ConVar *cv : cmds) {
        if (cv->get_name() == QStringLiteral("bind")) {
            cmd::ArgSpec spec;
            spec.index = 0;
            spec.isFloat = false;
            spec.hasMin = true;
            spec.min = 0.0;
            spec.hasMax = true;
            spec.max = 3.0;
            static_cast<cmd::ConCmd *>(cv)->setArgSpecs({spec});
        }
    }

    ConsoleCommandModel model;
    model.setCommands(cmds);

    // The model sorts rows, so find `bind` by name rather than trusting row 0.
    int bindRowIdx = -1;
    int sensRowIdx = -1;
    for (int r = 0; r < model.rowCount(); ++r) {
        const QString name = model.commandAt(r).value(QStringLiteral("name")).toString();
        if (name == QStringLiteral("bind"))
            bindRowIdx = r;
        if (name == QStringLiteral("sensitivity"))
            sensRowIdx = r;
    }
    QVERIFY(bindRowIdx >= 0);
    const QVariantMap bindRow = model.commandAt(bindRowIdx);
    const QVariantList bindSpec = bindRow.value(QStringLiteral("argsSpec")).toList();
    QCOMPARE(bindSpec.size(), 2);
    const QVariantMap first = bindSpec.at(0).toMap();
    QCOMPARE(first.value(QStringLiteral("index")).toInt(), 0);
    QVERIFY(!first.value(QStringLiteral("optional")).toBool());
    QVERIFY(first.value(QStringLiteral("hasMin")).toBool());
    QCOMPARE(first.value(QStringLiteral("min")).toString(), QStringLiteral("0"));
    QVERIFY(first.value(QStringLiteral("hasMax")).toBool());
    QCOMPARE(first.value(QStringLiteral("max")).toString(), QStringLiteral("3"));
    const QVariantMap second = bindSpec.at(1).toMap();
    QCOMPARE(second.value(QStringLiteral("index")).toInt(), 1);
    QVERIFY(!second.value(QStringLiteral("hasMin")).toBool());
    QVERIFY(!second.value(QStringLiteral("hasMax")).toBool());

    // findCommand carries the same payload, and a cvar carries none.
    QCOMPARE(model.findCommand(QStringLiteral("BIND"))
                 .value(QStringLiteral("argsSpec")).toList().size(), 2);
    QVERIFY(model.findCommand(QStringLiteral("sensitivity"))
                 .value(QStringLiteral("argsSpec")).toList().isEmpty());
    QVERIFY(sensRowIdx >= 0); // the cvar is visible; its role is empty, not unknown
    QVERIFY(model.commandAt(sensRowIdx).value(QStringLiteral("argsSpec")).toList().isEmpty());

    // An optional argument reaches the role as optional, and an unbounded one takes its
    // float-ness from the parsed kind ("<int>" is not float, "flt" is).
    cmd::ConCmd optionalArg(QStringLiteral("addcond"), QStringLiteral("Apply a condition."),
                            QStringLiteral("[int] <flt>"), QString(), QString(), QString());
    QVector<cmd::ConVar *> only;
    only << &optionalArg;
    ConsoleCommandModel optionalModel;
    optionalModel.setCommands(only);
    const QVariantList optionalSpecs =
        optionalModel.commandAt(0).value(QStringLiteral("argsSpec")).toList();
    QCOMPARE(optionalSpecs.size(), 2);
    QVERIFY(!optionalSpecs.at(0).toMap().value(QStringLiteral("optional")).toBool());
    QVERIFY(!optionalSpecs.at(0).toMap().value(QStringLiteral("isFloat")).toBool());
    QVERIFY(optionalSpecs.at(1).toMap().value(QStringLiteral("optional")).toBool());
    QVERIFY(optionalSpecs.at(1).toMap().value(QStringLiteral("isFloat")).toBool());
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

void EngineTest::setBindDropsLaterRowsForKey() {
    // A later unbind or bindtoggle row replays after a rebound bind in scanBinds() and
    // would otherwise override the new command (docs/AUDIT.md #1). setBind() drops every
    // later row for the key on the in-place path, matching the append path.
    CommandListModel model;
    model.setEntries(parse(QStringLiteral(
        "bind \"a\" \"+moveleft\"\nunbind \"a\"\nbindtoggle \"a\" \"+attack\"\n")));
    QCOMPARE(model.binds().value(QStringLiteral("a")).toString(), QStringLiteral("+attack"));
    QVERIFY(model.setBind(QStringLiteral("a"), QStringLiteral("+duck")));
    QCOMPARE(model.binds().value(QStringLiteral("a")).toString(), QStringLiteral("+duck"));
    QCOMPARE(model.rowCount(), 1);
}

void EngineTest::bindingKindReportsImportedToggle() {
    CommandListModel model;
    model.setEntries(parse(QStringLiteral("bind \"a\" \"+moveleft\"\nbindtoggle \"b\" \"+attack\"\n")));
    QCOMPARE(model.bindingKind(QStringLiteral("a")), QStringLiteral("bind"));
    QCOMPARE(model.bindingKind(QStringLiteral("b")), QStringLiteral("bindtoggle"));
    QCOMPARE(model.binds().value(QStringLiteral("b")).toString(), QStringLiteral("+attack"));
}

void EngineTest::setBindToggleWritesToggleRow() {
    CommandListModel model;
    QVERIFY(model.setBindToggle(QStringLiteral("k"), QStringLiteral("cl_showfps")));
    QCOMPARE(model.bindingKind(QStringLiteral("k")), QStringLiteral("bindtoggle"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), CommandListModel::CommandRole).toString(),
             QStringLiteral("bindtoggle"));
    QVERIFY(!model.setBindToggle(QStringLiteral("k"), QStringLiteral("cl_showfps")));
}

// --- ConsoleCommandModel ---

void EngineTest::catalogSortsByContextThenName() {
    CatalogFixture fixture;
    ConsoleCommandModel model;
    model.setCommands(fixture.make());

    // Primary key is the context (the picker's section grouping needs contiguous
    // groups), the old alphanumeric name key breaks ties within a group: attack,
    // jump | zapper | fov, sensitivity | bind.
    QStringList names;
    for (int row = 0; row < model.rowCount(); ++row)
        names << model.data(model.index(row, 0), ConsoleCommandModel::NameRole).toString();
    QCOMPARE(names, QStringList({ "+attack", "+jump", "zapper", "_fov", "sensitivity",
                                  "bind" }));
}

void EngineTest::contextAndPlatformFiltersNarrowRows() {
    CatalogFixture fixture;
    ConsoleCommandModel model;
    model.setCommands(fixture.make());

    model.setContextFilter(QStringLiteral("client controls"));
    QCOMPARE(model.rowCount(), 2);

    model.setPlatformFilter(QStringLiteral("linux"));
    QCOMPARE(model.rowCount(), 0); // nothing in client controls is linux-only

    model.setContextFilter(QStringLiteral("client render"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), ConsoleCommandModel::NameRole).toString(),
             QStringLiteral("zapper"));

    model.setPlatformFilter(QString());
    QCOMPARE(model.rowCount(), 1); // zapper, which is still client render

    model.setContextFilter(QString());
    QCOMPARE(model.rowCount(), 6);
}

void EngineTest::cheatRowsAreHiddenUnlessAsked() {
    CatalogFixture fixture;
    ConsoleCommandModel model;
    model.setCommands(fixture.make());

    model.setShowCheats(false);
    QCOMPARE(model.rowCount(), 5);
    model.setContextFilter(QStringLiteral("client controls"));
    QCOMPARE(model.rowCount(), 1); // +jump is flagged cheat
    QCOMPARE(model.data(model.index(0, 0), ConsoleCommandModel::NameRole).toString(),
             QStringLiteral("+attack"));

    model.setShowCheats(true);
    QCOMPARE(model.rowCount(), 2);
}

void EngineTest::filtersSurviveBeingSwitchedBackAndForth() {
    // The visible list is cached per filter combo, so a stale entry would show here.
    CatalogFixture fixture;
    ConsoleCommandModel model;
    model.setCommands(fixture.make());

    model.setContextFilter(QStringLiteral("client settings"));
    QCOMPARE(model.rowCount(), 2);
    model.setContextFilter(QString());
    QCOMPARE(model.rowCount(), 6);
    model.setPlatformFilter(QStringLiteral("linux"));
    QCOMPARE(model.rowCount(), 1);
    model.setContextFilter(QStringLiteral("client settings"));
    QCOMPARE(model.rowCount(), 0); // _fov and sensitivity are universal
}

void EngineTest::suggestOnlyOffersVisibleNames() {
    CatalogFixture fixture;
    ConsoleCommandModel model;
    model.setCommands(fixture.make());

    // Follows the sorted visible order and stops at the limit.
    QCOMPARE(model.suggest(QStringLiteral("a"), 5),
             QStringList({ "+attack", "zapper" }));
    QCOMPARE(model.suggest(QStringLiteral("ATT"), 5), QStringList({ "+attack" }));

    model.setContextFilter(QStringLiteral("client settings"));
    QCOMPARE(model.suggest(QStringLiteral("a"), 5), QStringList());
    QCOMPARE(model.suggest(QStringLiteral("S"), 5), QStringList({ "sensitivity" }));

    model.setContextFilter(QString());
    QCOMPARE(model.suggest(QStringLiteral("a"), 1), QStringList({ "+attack" }));
}

void EngineTest::suggestNeedsSomeText() {
    CatalogFixture fixture;
    ConsoleCommandModel model;
    model.setCommands(fixture.make());
    QVERIFY(model.suggest(QString(), 5).isEmpty());
    QVERIFY(model.suggest(QStringLiteral("nothing-matches-this"), 5).isEmpty());
}

void EngineTest::findCommandIgnoresCaseAndFilters() {
    CatalogFixture fixture;
    ConsoleCommandModel model;
    model.setCommands(fixture.make());
    model.setContextFilter(QStringLiteral("client settings"));

    // Looked up in the whole catalog, not the filtered list: the Build-A-Bind pane
    // resolves a typed name that its own filters would hide.
    const QVariantMap found = model.findCommand(QStringLiteral("SENSITIVITY"));
    QCOMPARE(found.value(QStringLiteral("name")).toString(), QStringLiteral("sensitivity"));
    QCOMPARE(found.value(QStringLiteral("context")).toString(), QStringLiteral("client settings"));
    QVERIFY(model.findCommand(QStringLiteral("no_such_command")).isEmpty());
}

void EngineTest::findCommandCarriesTheFullRecord() {
    // The Config Editor's guided pane (docs/DESIGN-config-editor.md §4.1) fills its
    // editors from findCommand, so the record must carry the cvar metadata too, not
    // only the name/context/platform the bind dialog needed.
    CatalogFixture fixture;
    ConsoleCommandModel model;
    model.setCommands(fixture.make());

    const QVariantMap cvar = model.findCommand(QStringLiteral("SENSITIVITY"));
    QCOMPARE(cvar.value(QStringLiteral("input")).toString(), QStringLiteral("flt"));
    QCOMPARE(cvar.value(QStringLiteral("def")).toString(), QStringLiteral("3"));
    QCOMPARE(cvar.value(QStringLiteral("min")).toString(), QStringLiteral("0.0001"));
    QCOMPARE(cvar.value(QStringLiteral("max")).toString(), QStringLiteral("10000000"));
    QVERIFY(cvar.value(QStringLiteral("isCvar")).toBool());
    QCOMPARE(cvar.value(QStringLiteral("docs")).toString(), QStringLiteral("Mouse sensitivity."));

    // A command is not a cvar; its signature is what the per-token editors parse.
    const QVariantMap cmd = model.findCommand(QStringLiteral("bind"));
    QVERIFY(!cmd.value(QStringLiteral("isCvar")).toBool());
    QCOMPARE(cmd.value(QStringLiteral("input")).toString(), QStringLiteral("[str] [str]"));
    QVERIFY(model.findCommand(QStringLiteral("nope")).isEmpty());
}

void EngineTest::commandAtCarriesTheArgumentMetadata() {
    CatalogFixture fixture;
    ConsoleCommandModel model;
    model.setCommands(fixture.make());

    // Rows are located by name: the sort order is its own test (rowsSortByContextThenName).
    int sensRowIdx = -1;
    int bindRowIdx = -1;
    for (int r = 0; r < model.rowCount(); ++r) {
        const QString name = model.commandAt(r).value(QStringLiteral("name")).toString();
        if (name == QStringLiteral("sensitivity"))
            sensRowIdx = r;
        if (name == QStringLiteral("bind"))
            bindRowIdx = r;
    }
    QVERIFY(sensRowIdx >= 0);
    QVERIFY(bindRowIdx >= 0);

    const QVariantMap flt = model.commandAt(sensRowIdx);
    QCOMPARE(flt.value(QStringLiteral("def")).toString(), QStringLiteral("3"));
    // Whole numbers stay whole: QString::number's default format would give "1e+07" here,
    // which is what the argument editor's spin box would then show.
    QCOMPARE(flt.value(QStringLiteral("min")).toString(), QStringLiteral("0.0001"));
    QCOMPARE(flt.value(QStringLiteral("max")).toString(), QStringLiteral("10000000"));
    QVERIFY(flt.value(QStringLiteral("isCvar")).toBool());

    // A command with a signature is not a cvar, so the pane shows no argument editors.
    const QVariantMap cmd = model.commandAt(bindRowIdx);
    QCOMPARE(cmd.value(QStringLiteral("input")).toString(), QStringLiteral("[str] [str]"));
    QVERIFY(!cmd.value(QStringLiteral("isCvar")).toBool());

    QVERIFY(model.commandAt(-1).isEmpty());
    QVERIFY(model.commandAt(99).isEmpty());
}

// --- ConfigController: the surgical save ---

/// A loaded controller over `path`. loadCfg parses on a worker thread; the tests drive
/// the controller synchronously, so spin until the queued model update lands. A macro
/// wrapper, because QTRY_COMPARE contains `return` and cannot live in a function.
#define LOAD_CONFIG(controller, path) \
    do { \
        (controller).loadCfg(path); \
        QTRY_COMPARE((controller).loaded(), true); \
    } while (false)

void EngineTest::saveRewritesOnlyOwnedLines() {
    // Comments, blank lines, an alias and a cvar line must survive byte-for-byte; only
    // the bind lines the app touched are re-rendered.
    const QString cfgText = QStringLiteral(
        "// my binds, edited by hand\n"
        "unbindall\n"
        "bind \"a\" \"+moveleft\"\n"
        "\n"
        "bind \"b\" \"+jump\"\n"
        "// a cvar below, not a bind\n"
        "volume \"0.7\"\n"
        "alias \"+quickswitch\" \"slot3; slot1\"\n");
    const QString path = writeFile(QStringLiteral("owned.cfg"), cfgText);

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(controller.setBind(QStringLiteral("b"), QStringLiteral("+duck")));
    QVERIFY(controller.saveCfg());

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QStringList lines = QString::fromUtf8(saved.readAll()).split(QLatin1Char('\n'));
    saved.close();

    // Every line that is not an app-owned bind is unchanged, terminators aside.
    QCOMPARE(lines.at(0), QStringLiteral("// my binds, edited by hand"));
    QCOMPARE(lines.at(1), QStringLiteral("unbindall"));
    QCOMPARE(lines.at(2), QStringLiteral("bind \"a\" \"+moveleft\""));
    QCOMPARE(lines.at(3), QString()); // the blank line survived
    QCOMPARE(lines.at(4), QStringLiteral("bind \"b\" \"+duck\"")); // re-rendered
    QCOMPARE(lines.at(5), QStringLiteral("// a cvar below, not a bind"));
    QCOMPARE(lines.at(6), QStringLiteral("volume \"0.7\""));
    QCOMPARE(lines.at(7), QStringLiteral("alias \"+quickswitch\" \"slot3; slot1\""));
}

void EngineTest::savePreservesCrlfAndMissingFinalNewline() {
    // CRLF endings and a final line with no newline: the save must not normalize either.
    const QByteArray original = "unbindall\r\nbind \"a\" \"+moveleft\"\r\nsv_cheats 1";
    const QString path = writeFile(QStringLiteral("crlf.cfg"), QString::fromUtf8(original));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(controller.setBind(QStringLiteral("a"), QStringLiteral("+duck")));
    QVERIFY(controller.saveCfg());

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly));
    const QByteArray bytes = saved.readAll();
    saved.close();

    // \r\n stays \r\n, and the last line still has no terminator.
    QVERIFY(bytes.startsWith("unbindall\r\n"));
    QVERIFY(bytes.contains("bind \"a\" \"+duck\"\r\n"));
    QVERIFY(bytes.endsWith("sv_cheats 1"));
    QCOMPARE(bytes.count("\r\n"), 2);
    QVERIFY(!bytes.contains("\n\n"));
}

void EngineTest::saveDropsDeletedBinds() {
    const QString path = writeFile(QStringLiteral("drops.cfg"),
                                   QStringLiteral("bind \"a\" \"+moveleft\"\nsv_cheats 1\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(controller.clearBind(QStringLiteral("a"))); // removes the row
    QVERIFY(controller.saveCfg());

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString text = QString::fromUtf8(saved.readAll());
    saved.close();
    QVERIFY(!text.contains(QLatin1String("+moveleft")));
    QVERIFY(text.contains(QLatin1String("sv_cheats 1")));
}

void EngineTest::saveAppendsSessionBindsAfterTheOwnedBlock() {
    // A bind added in-app has no source line: it lands after the last bind line, so the
    // binds stay together instead of scattering to the end of the file.
    const QString path = writeFile(QStringLiteral("session.cfg"),
                                   QStringLiteral("bind \"a\" \"+moveleft\"\n"
                                                  "sv_cheats 1\n"
                                                  "alias \"x\" \"say hi\"\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(controller.setBind(QStringLiteral("k"), QStringLiteral("+jump")));
    QVERIFY(controller.saveCfg());

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QStringList lines = QString::fromUtf8(saved.readAll()).split(QLatin1Char('\n'));
    saved.close();

    QCOMPARE(lines.at(0), QStringLiteral("bind \"a\" \"+moveleft\""));
    QCOMPARE(lines.at(1), QStringLiteral("bind \"k\" \"+jump\"")); // right after the binds
    QCOMPARE(lines.at(2), QStringLiteral("sv_cheats 1"));
    QCOMPARE(lines.at(3), QStringLiteral("alias \"x\" \"say hi\""));
}

void EngineTest::saveIsIdempotent() {
    const QString path = writeFile(QStringLiteral("idem.cfg"),
                                   QStringLiteral("// bind block\nbind \"a\" \"+moveleft\"\nsv_cheats 1\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(controller.setBind(QStringLiteral("a"), QStringLiteral("+duck")));
    QVERIFY(controller.saveCfg());

    QFile first(path);
    QVERIFY(first.open(QIODevice::ReadOnly));
    const QByteArray afterFirst = first.readAll();
    first.close();

    QVERIFY(controller.saveCfg()); // second save over the first
    QFile second(path);
    QVERIFY(second.open(QIODevice::ReadOnly));
    const QByteArray afterSecond = second.readAll();
    second.close();

    QCOMPARE(afterSecond, afterFirst);
}

void EngineTest::saveWritesABakBesideTheFile() {
    const QString path = writeFile(QStringLiteral("bak.cfg"),
                                   QStringLiteral("bind \"a\" \"+moveleft\"\n"));
    const QString bak = path + QStringLiteral(".bak");

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(controller.setBind(QStringLiteral("a"), QStringLiteral("+duck")));
    QVERIFY(controller.saveCfg());

    QFile backup(bak);
    QVERIFY2(backup.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(bak));
    const QString text = QString::fromUtf8(backup.readAll());
    backup.close();
    // The .bak holds what was on disk before this save: the untouched original.
    QCOMPARE(text, QStringLiteral("bind \"a\" \"+moveleft\"\n"));
}

void EngineTest::failedBackupWriteKeepsTheOldRestorePoint() {
    // DESIGN-save-backup.md failure semantics: a failed backup copy changes nothing -
    // the old .bak survives and the main file is untouched (the pre-1201dff code
    // removed .bak before copying, destroying the only restore point).
    const QString path = writeFile(QStringLiteral("bakfail.cfg"),
                                   QStringLiteral("volume 0.5\n"));
    const QString bak = path + QStringLiteral(".bak");
    QVERIFY(QFile::copy(path, bak)); // seed a previous restore point
    // Sabotage the copy: a directory at the `.bak.new` path makes QFile::copy fail.
    QVERIFY(QDir(m_dir.path()).mkdir(QStringLiteral("bakfail.cfg.bak.new")));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(controller.setBind(QStringLiteral("a"), QStringLiteral("+duck")));
    QVERIFY(!controller.saveCfg());
    QVERIFY(controller.lastError().contains(QStringLiteral("backup")));

    QFile main(path);
    QVERIFY(main.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(main.readAll()), QStringLiteral("volume 0.5\n"));
    main.close();
    QFile restore(bak);
    QVERIFY2(restore.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(bak));
    QCOMPARE(QString::fromUtf8(restore.readAll()), QStringLiteral("volume 0.5\n"));
    restore.close();
    QVERIFY(!QFile::exists(bak + QStringLiteral(".old")));
    QVERIFY(QDir(m_dir.path()).rmdir(QStringLiteral("bakfail.cfg.bak.new")));
}

// --- KeyTable (deviation D9) ---

void EngineTest::keyTableParityInvariants() {
    // The parity contract with the retired widget build (keybform.h): ids are
    // unique (0..132 with -1 for unbindables), unbindable caps have no token, and
    // every layout/cell name resolves to a real cap.
    KeyTable table;

    QHash<int, QString> seenIds;
    const QVariantList groups = table.fnRowGroups() + table.mainRows()
                                + table.middleRows() + table.numpadMain()
                                + table.numpadRight();
    int named = 0;
    for (const QVariant &g : groups) {
        if (g.isNull())
            continue; // the deliberate blank slot above the arrow keys
        for (const QVariant &name : g.toList()) {
            ++named;
            const QVariantMap d = table.keyData(name.toString());
            QVERIFY2(!d.isEmpty(), qPrintable(name.toString()));
            const int id = d.value(QStringLiteral("id")).toInt();
            if (id >= 0)
                QVERIFY2(!seenIds.contains(id),
                         qPrintable(QStringLiteral("duplicate id %1").arg(id)));
            seenIds.insert(id, name.toString());
            if (!d.value(QStringLiteral("bindable"), true).toBool()) {
                QCOMPARE(id, -1);
                QCOMPARE(d.value(QStringLiteral("token")).toString(), QString());
            } else {
                QVERIFY(!d.value(QStringLiteral("token")).toString().isEmpty());
                QCOMPARE(table.nameForToken(d.value(QStringLiteral("token")).toString()),
                         name.toString());
            }
        }
    }
    QVERIFY(named > 100); // the full keyboard is present

    // Mouse/controller cells reference real caps and carry sane geometry.
    for (const QVariantList cells : {table.mouseCells(), table.controllerCells()}) {
        for (const QVariant &v : cells) {
            const QVariantMap c = v.toMap();
            QVERIFY2(!table.keyData(c.value(QStringLiteral("name")).toString()).isEmpty(),
                     qPrintable(c.value(QStringLiteral("name")).toString()));
            QVERIFY(c.value(QStringLiteral("w")).toInt() > 0);
            QVERIFY(c.value(QStringLiteral("h")).toInt() > 0);
        }
    }
    QVERIFY(table.mouseWidth() > 0 && table.controllerWidth() > 0);

    // Spot parity against the retired QML singleton's data.
    QCOMPARE(table.keyId(QStringLiteral("space")), 71);
    QCOMPARE(table.keyToken(QStringLiteral("space")), QStringLiteral("space"));
    QCOMPARE(table.keyToken(QStringLiteral("kp0")), QStringLiteral("kp_ins"));
    QCOMPARE(table.keyId(QStringLiteral("menu")), -1);
    QVERIFY(!table.isBindable(QStringLiteral("menu")));
    QCOMPARE(table.tokenForId(0), QStringLiteral("escape"));
    QCOMPARE(table.keyLabel(QStringLiteral("kpslsh")), QStringLiteral("KP /"));

    // Geometry defaults flow through from the caller's theme values: a standard
    // cap is capSize wide, the wide caps carry their own width, and a two-row cap
    // spans rows + gap. (Regression: the first C++ port returned 0/row-counts and
    // every keyboard cap rendered at zero size.)
    QCOMPARE(table.keyWidth(QStringLiteral("esc"), 64), 64);
    QCOMPARE(table.keyWidth(QStringLiteral("bkspc"), 64), 128);
    QCOMPARE(table.keyHeight(QStringLiteral("esc"), 64, 4), 64);
    QCOMPARE(table.keyHeight(QStringLiteral("kpplus"), 64, 4), 132);
}

void EngineTest::saveRefusesAResourcePath() {
    // The bundled presets are Qt resources: they "exist" but cannot be written.
    ConfigController controller;
    LOAD_CONFIG(controller, QStringLiteral(":/cfg/default.cfg"));
    QVERIFY(!controller.canSave());
    QVERIFY(!controller.saveBlockReason().isEmpty());
    QVERIFY(controller.saveCfg() == false); // refuses, with lastError set
    QVERIFY(!controller.lastError().isEmpty());

    // Save As to a real path works and re-points cfgPath at the file. The surgical
    // baseline is the loaded preset, so the target must hold the preset's content -
    // an empty file here would mean the save lost everything not app-owned.
    const QString target = m_dir.filePath(QStringLiteral("saved-from-preset.cfg"));
    QVERIFY(controller.saveCfgAs(target));
    QCOMPARE(controller.cfgPath(), target);
    QFile savedPreset(target);
    QVERIFY(savedPreset.open(QIODevice::ReadOnly));
    const QString savedText = QString::fromUtf8(savedPreset.readAll());
    savedPreset.close();
    QFile preset(QStringLiteral(":/cfg/default.cfg"));
    QVERIFY(preset.open(QIODevice::ReadOnly));
    const QString presetText = QString::fromUtf8(preset.readAll());
    preset.close();
    QCOMPARE(savedText, presetText);
    QVERIFY(!savedText.isEmpty());
    QVERIFY(!controller.isDirty());

    // The saved-as path is now the source for further editing and saving. Bind a key
    // that the preset does not bind, save, then rebind it: the second save must rewrite
    // the line the first save inserted, not add a second one beside it - which is what
    // happens when the model still indexes the old (preset) layout.
    QVERIFY(controller.setBind(QStringLiteral("k"), QStringLiteral("+jump")));
    QVERIFY(controller.saveCfg());
    QVERIFY(!controller.isDirty()); // the resync cleared app ownership again

    QFile second(target);
    QVERIFY(second.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString afterFirst = QString::fromUtf8(second.readAll());
    second.close();
    QCOMPARE(afterFirst.count(QLatin1String("bind \"k\" \"+jump\"")), 1);

    QVERIFY(controller.setBind(QStringLiteral("k"), QStringLiteral("+duck")));
    QVERIFY(controller.saveCfg());
    QFile third(target);
    QVERIFY(third.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString afterSecond = QString::fromUtf8(third.readAll());
    third.close();
    QCOMPARE(afterSecond.count(QLatin1String("bind \"k\"")), 1);
    QVERIFY(afterSecond.contains(QLatin1String("bind \"k\" \"+duck\"")));
}

void EngineTest::saveRecreatesAVanishedFile() {
    // The file is gone when the (confirmed) save runs: with no baseline to copy from,
    // the model's rows are all the knowledge there is and must all reach the file,
    // unowned parsed ones included - not just the app-owned lines.
    const QString path = writeFile(QStringLiteral("vanish.cfg"),
                                   QStringLiteral("bind \"a\" \"+moveleft\"\nsv_cheats 1\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(QFile::remove(path));
    QVERIFY(controller.needsConfirm());
    QVERIFY(controller.saveCfg());

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString text = QString::fromUtf8(saved.readAll());
    saved.close();
    QCOMPARE(text, QStringLiteral("bind \"a\" \"+moveleft\"\nsv_cheats 1\n"));
}

void EngineTest::saveDetectsAnExternallyModifiedFile() {
    const QString path = writeFile(QStringLiteral("ext.cfg"),
                                   QStringLiteral("bind \"a\" \"+moveleft\"\nsv_cheats 1\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(!controller.needsConfirm()); // just loaded, nothing changed

    // Touch the file the way a sync tool would.
    {
        QFile touch(path);
        QVERIFY(touch.open(QIODevice::WriteOnly | QIODevice::Text));
        touch.write("bind \"a\" \"+moveleft\"\nsv_cheats 1\n// sync tool comment\n");
        touch.close();
    }
    QVERIFY2(controller.needsConfirm(), "an externally modified file must ask before saving");
    QVERIFY(controller.saveCfg()); // the write itself still works once confirmed

    // And after the save the stamp is re-recorded, so the next save is silent again.
    QVERIFY(!controller.needsConfirm());
}

// --- Config Editor (docs/DESIGN-config-editor.md, chunk A) ---

void EngineTest::parseCommandLineTokenizesLikeTheLexer() {
    // The reverse-parse runs the same lexer as the file parser, so custom text and
    // parsed rows are one tokenizer apart (§8) - quotes stay on, exactly like cfgLex.
    ConfigController controller;
    const QVariantMap parsed = controller.parseCommandLine(
        QStringLiteral("bind \"a\" \"+moveleft\""));
    QCOMPARE(parsed.value(QStringLiteral("ok")).toBool(), true);
    QCOMPARE(parsed.value(QStringLiteral("typeName")).toString(), QStringLiteral("command"));
    QCOMPARE(parsed.value(QStringLiteral("args")).toList().size(), 3);
    QCOMPARE(parsed.value(QStringLiteral("args")).toList().at(2).toString(),
             QStringLiteral("\"+moveleft\""));

    // An empty line and a comment-only line are refused: there is nothing to edit.
    QVERIFY(controller.parseCommandLine(QString()).value(QStringLiteral("ok")).toBool() == false);
    QVERIFY(controller.parseCommandLine(QStringLiteral("// note"))
                .value(QStringLiteral("ok")).toBool() == false);

    // A head the catalog does not know stays unknown (§3: it may be an alias defined
    // in another cfg the game executes at runtime).
    QCOMPARE(controller.parseCommandLine(QStringLiteral("zzz_not_a_cmd 1"))
                 .value(QStringLiteral("typeName")).toString(), QStringLiteral("unknown"));
}

void EngineTest::setRowTextReclassifiesAndOwnsTheRow() {
    const QString path = writeFile(QStringLiteral("setrow.cfg"),
                                   QStringLiteral("// comment\nvolume \"0.7\"\nbind \"a\" \"+moveleft\"\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    // Rows: 0 standalone comment, 1 volume, 2 bind.
    QVERIFY(controller.setRowText(1, QStringLiteral("volume 0.9")));
    QVERIFY(controller.isDirty());

    const QVariantMap info = controller.rowInfo(1);
    QCOMPARE(info.value(QStringLiteral("line")).toString(), QStringLiteral("volume 0.9"));
    QCOMPARE(info.value(QStringLiteral("typeName")).toString(), QStringLiteral("float"));
    QVERIFY(info.value(QStringLiteral("warning")).toString().isEmpty());

    QVERIFY(controller.saveCfg());
    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QStringList lines = QString::fromUtf8(saved.readAll()).split(QLatin1Char('\n'));
    saved.close();
    // The edited line is re-rendered in place; everything else survives byte-for-byte.
    QCOMPARE(lines.at(0), QStringLiteral("// comment"));
    QCOMPARE(lines.at(1), QStringLiteral("volume 0.9"));
    QCOMPARE(lines.at(2), QStringLiteral("bind \"a\" \"+moveleft\""));

    // An empty or comment-only replacement is refused and changes nothing.
    QVERIFY(!controller.setRowText(1, QString()));
    QVERIFY(!controller.setRowText(1, QStringLiteral("// only a comment")));
    QCOMPARE(controller.rowInfo(1).value(QStringLiteral("line")).toString(),
             QStringLiteral("volume 0.9"));
}

void EngineTest::deleteRowDropsItsLineOnSave() {
    const QString path = writeFile(QStringLiteral("delrow.cfg"),
                                   QStringLiteral("sv_cheats 1\nvolume 0.5\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(controller.deleteRow(0));
    QVERIFY(controller.isDirty());
    QVERIFY(controller.saveCfg());

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString text = QString::fromUtf8(saved.readAll());
    saved.close();
    QVERIFY(!text.contains(QLatin1String("sv_cheats")));
    QCOMPARE(text, QStringLiteral("volume 0.5\n"));
}

void EngineTest::insertRowAfterLandsAfterTheAnchorLine() {
    const QString path = writeFile(QStringLiteral("insrow.cfg"),
                                   QStringLiteral("volume 0.5\nsv_cheats 1\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    // Insert after the selected parsed row: the new line lands right below it.
    QCOMPARE(controller.insertRowAfter(0, QStringLiteral("echo hi")), 1);
    // No anchor: end of file.
    QCOMPARE(controller.insertRowAfter(-1, QStringLiteral("echo eof")), 3);
    QVERIFY(controller.isDirty());
    QVERIFY(controller.saveCfg());

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QStringList lines = QString::fromUtf8(saved.readAll()).split(QLatin1Char('\n'));
    saved.close();
    QCOMPARE(lines.at(0), QStringLiteral("volume 0.5"));
    QCOMPARE(lines.at(1), QStringLiteral("echo hi"));
    QCOMPARE(lines.at(2), QStringLiteral("sv_cheats 1"));
    QCOMPARE(lines.at(3), QStringLiteral("echo eof"));
}

void EngineTest::insertRowAfterChainsSessionAnchors() {
    // Inserting "after" a row that is itself a session insert keeps stacking below it,
    // so a run of added lines stays in the order the user built them (§9).
    const QString path = writeFile(QStringLiteral("chainrow.cfg"),
                                   QStringLiteral("volume 0.5\nsv_cheats 1\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QCOMPARE(controller.insertRowAfter(0, QStringLiteral("echo a")), 1);
    QCOMPARE(controller.insertRowAfter(1, QStringLiteral("echo b")), 2);
    QVERIFY(controller.saveCfg());

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QStringList lines = QString::fromUtf8(saved.readAll()).split(QLatin1Char('\n'));
    saved.close();
    QCOMPARE(lines.at(1), QStringLiteral("echo a"));
    QCOMPARE(lines.at(2), QStringLiteral("echo b"));
    QCOMPARE(lines.at(3), QStringLiteral("sv_cheats 1"));
}

void EngineTest::warningsFlagDeadBindsAndOverriddenCvars() {
    // §7: an unbind (or unbindall) after a bind kills it; a cvar set again leaves the
    // earlier set dead. The warning sits on the row that never takes effect.
    CommandListModel model;
    model.setEntries(parse(QStringLiteral(
        "bind \"a\" \"+moveleft\"\n"
        "bind \"b\" \"+jump\"\n"
        "unbind \"a\"\n"
        "sensitivity 2.5\n"
        "sensitivity 3\n")));
    QCOMPARE(model.rowCount(), 5);
    QVERIFY(model.data(model.index(0, 0), CommandListModel::WarningRole)
                .toString().contains(QLatin1String("unbind")));
    QVERIFY(model.data(model.index(1, 0), CommandListModel::WarningRole).toString().isEmpty());
    QVERIFY(model.data(model.index(2, 0), CommandListModel::WarningRole).toString().isEmpty());
    QVERIFY(model.data(model.index(3, 0), CommandListModel::WarningRole)
                .toString().contains(QLatin1String("overridden")));
    QVERIFY(model.data(model.index(4, 0), CommandListModel::WarningRole).toString().isEmpty());

    // An earlier bind row plus a trailing unbindall: the bind is flagged, and a bind
    // re-added after the unbindall is not.
    CommandListModel afterAll;
    afterAll.setEntries(parse(QStringLiteral(
        "bind \"a\" \"+moveleft\"\nunbindall\nbind \"c\" \"+jump\"\n")));
    QVERIFY(afterAll.data(afterAll.index(0, 0), CommandListModel::WarningRole)
                .toString().contains(QLatin1String("unbindall")));
    QVERIFY(afterAll.data(afterAll.index(2, 0), CommandListModel::WarningRole)
                .toString().isEmpty());
}

void EngineTest::filterModelKindsAndText() {
    CommandListModel model;
    model.setEntries(parse(QStringLiteral(
        "volume 0.5\n"
        "bind \"a\" \"+moveleft\"\n"
        "alias \"quick\" \"slot3; slot1\"\n"
        "zzz_unknown 1\n")));
    CommandFilterModel proxy;
    proxy.setSourceModel(&model);

    QCOMPARE(proxy.rowCount(), 4);

    proxy.setFilterKind(0); // cvars
    QCOMPARE(proxy.rowCount(), 1);
    proxy.setFilterKind(1); // commands (binds have their own kind)
    QCOMPARE(proxy.rowCount(), 1); // the alias definition row parses as a command
    proxy.setFilterKind(2); // aliases (definition or reference)
    QCOMPARE(proxy.rowCount(), 1);
    proxy.setFilterKind(3); // not in catalog
    QCOMPARE(proxy.rowCount(), 1);
    proxy.setFilterKind(4); // binds (redirect targets)
    QCOMPARE(proxy.rowCount(), 1);
    proxy.setFilterKind(-1);

    proxy.setFilterText(QStringLiteral("VOL")); // case-insensitive on the row text
    QCOMPARE(proxy.rowCount(), 1);
    proxy.setFilterText(QStringLiteral("nosuchtext"));
    QCOMPARE(proxy.rowCount(), 0);
    proxy.setFilterText(QString());
    QCOMPARE(proxy.rowCount(), 4);

    // RowRole passes through unrenumbered, so the editor can edit by source row.
    proxy.setFilterText(QStringLiteral("bind"));
    QCOMPARE(proxy.data(proxy.index(0, 0), CommandListModel::RowRole).toInt(), 1);
}

// --- Aliases + chains (docs/DESIGN-config-editor.md §5, chunk C) ---

void EngineTest::setAliasCreatesARoundTrippingRow() {
    const QString path = writeFile(QStringLiteral("aliasnew.cfg"),
                                   QStringLiteral("bind \"a\" \"+moveleft\"\nsv_cheats 1\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(controller.setAlias(QStringLiteral("quickswitch"), QStringLiteral("slot3; slot1")));
    QVERIFY(controller.isDirty());
    QVERIFY(controller.saveCfg());

    // No alias lines existed, so the definition lands at end of file (the presets
    // define aliases after their binds).
    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QStringList lines = QString::fromUtf8(saved.readAll()).split(QLatin1Char('\n'));
    saved.close();
    QCOMPARE(lines.at(2), QStringLiteral("alias \"quickswitch\" \"slot3; slot1\""));

    // The saved row re-parses to the same body, so the model and the file agree.
    const QVariantMap info = controller.aliasInfo(QStringLiteral("QUICKSWITCH"));
    QCOMPARE(info.value(QStringLiteral("found")).toBool(), true);
    QCOMPARE(info.value(QStringLiteral("body")).toString(), QStringLiteral("slot3; slot1"));
    QVERIFY(controller.aliasNames().contains(QLatin1String("quickswitch")));
}

void EngineTest::setAliasEditsInPlaceAndDropsDuplicates() {
    const QString path = writeFile(QStringLiteral("aliasedit.cfg"),
                                   QStringLiteral("alias \"x\" \"one\"\n"
                                                  "bind \"a\" \"+moveleft\"\n"
                                                  "alias \"x\" \"two\"\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(controller.setAlias(QStringLiteral("X"), QStringLiteral("three")));
    QVERIFY(controller.saveCfg());

    // The latest definition is rewritten where it stood; the earlier duplicate (dead
    // weight under TF2's replay) is dropped, its line gone.
    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString text = QString::fromUtf8(saved.readAll());
    saved.close();
    QVERIFY(!text.contains(QLatin1String("\"one\"")));
    QVERIFY(!text.contains(QLatin1String("\"two\"")));
    QCOMPARE(text, QStringLiteral("bind \"a\" \"+moveleft\"\nalias \"x\" \"three\"\n"));

    // A body with quotes survives the quote-escape round trip (D19).
    QVERIFY(controller.setAlias(QStringLiteral("t"), QStringLiteral("say \"hi\"")));
    QVERIFY(controller.saveCfg());
    QFile quoted(path);
    QVERIFY(quoted.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString quotedText = QString::fromUtf8(quoted.readAll());
    quoted.close();
    QVERIFY(quotedText.contains(QLatin1String("alias \"t\" \"say \\\"hi\\\"\"")));
    QCOMPARE(controller.aliasInfo(QStringLiteral("t")).value(QStringLiteral("body")).toString(),
             QStringLiteral("say \"hi\""));
}

void EngineTest::setAliasEmptyBodyRemoves() {
    const QString path = writeFile(QStringLiteral("aliasrm.cfg"),
                                   QStringLiteral("alias \"x\" \"one\"\nsv_cheats 1\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(controller.setAlias(QStringLiteral("x"), QString()));
    QVERIFY(controller.isDirty());
    QVERIFY(controller.saveCfg());

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString text = QString::fromUtf8(saved.readAll());
    saved.close();
    QCOMPARE(text, QStringLiteral("sv_cheats 1\n"));
    QVERIFY(!controller.setAlias(QStringLiteral("x"), QString())); // nothing left to remove
}

void EngineTest::setAliasAnchorsAfterTheLastAliasLine() {
    const QString path = writeFile(QStringLiteral("aliasanchor.cfg"),
                                   QStringLiteral("bind \"a\" \"+moveleft\"\n"
                                                  "alias \"x\" \"one\"\n"
                                                  "sv_cheats 1\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    QVERIFY(controller.setAlias(QStringLiteral("y"), QStringLiteral("two")));
    QVERIFY(controller.saveCfg());

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QStringList lines = QString::fromUtf8(saved.readAll()).split(QLatin1Char('\n'));
    saved.close();
    // The new definition lands with the other aliases, not scattered to the end.
    QCOMPARE(lines.at(1), QStringLiteral("alias \"x\" \"one\""));
    QCOMPARE(lines.at(2), QStringLiteral("alias \"y\" \"two\""));
    QCOMPARE(lines.at(3), QStringLiteral("sv_cheats 1"));
}

void EngineTest::splitSegmentsIsQuoteAware() {
    ConfigController controller;
    // A ';' inside a quoted token does not split.
    QCOMPARE(controller.splitSegments(QStringLiteral("bind \"a\" \"x; y\"; volume 0.5")),
             QStringList({ QStringLiteral("bind \"a\" \"x; y\""),
                           QStringLiteral("volume 0.5") }));
    QCOMPARE(controller.splitSegments(QStringLiteral("echo hi")),
             QStringList({ QStringLiteral("echo hi") }));
    // Empty pieces from stray separators drop; an escaped quote stays inside.
    QCOMPARE(controller.splitSegments(QStringLiteral("a ; ; b")),
             QStringList({ QStringLiteral("a"), QStringLiteral("b") }));
    QVERIFY(controller.splitSegments(QStringLiteral("")).isEmpty());
}

void EngineTest::quoteUnquoteRoundTrips() {
    ConfigController controller;
    // The D19 pair: quoteToken writes what unquoteToken reads back identically.
    const QString raw = QStringLiteral("say \"hi\" \\ path");
    QCOMPARE(controller.unquoteToken(controller.quoteToken(raw)), raw);
    QCOMPARE(controller.unquoteToken(QStringLiteral("\"slot3\"")), QStringLiteral("slot3"));
}

// --- Comments (docs/DESIGN-config-editor.md §6, chunk D) ---

void EngineTest::commentRowsParseAndRoundTrip() {
    const QString path = writeFile(QStringLiteral("comments.cfg"),
                                   QStringLiteral("// header note\n"
                                                  "sv_cheats 1 // cheats on\n"
                                                  "volume 0.5\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    // Rows: 0 standalone comment, 1 sv_cheats with tail, 2 volume.
    QCOMPARE(controller.rowInfo(0).value(QStringLiteral("typeName")).toString(),
             QStringLiteral("comment"));
    QCOMPARE(controller.rowInfo(0).value(QStringLiteral("line")).toString(),
             QStringLiteral("// header note"));
    QCOMPARE(controller.rowInfo(1).value(QStringLiteral("line")).toString(),
             QStringLiteral("sv_cheats 1 // cheats on"));

    // Edit the standalone comment: raw text, no tokenizing; a missing `//` is
    // restored so the row still reads back as a comment after a resync.
    QVERIFY(controller.setRowText(0, QStringLiteral("edited note")));
    QCOMPARE(controller.rowInfo(0).value(QStringLiteral("line")).toString(),
             QStringLiteral("// edited note"));

    // Insert a comment line, then delete it again: net zero.
    const int inserted = controller.insertRowAfter(2, QStringLiteral("// new note"));
    QVERIFY(inserted >= 0);
    QCOMPARE(controller.rowInfo(inserted).value(QStringLiteral("typeName")).toString(),
             QStringLiteral("comment"));
    QVERIFY(controller.deleteRow(inserted));

    QVERIFY(controller.saveCfg());
    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const QStringList lines = QString::fromUtf8(saved.readAll()).split(QLatin1Char('\n'));
    saved.close();
    QCOMPARE(lines.at(0), QStringLiteral("// edited note"));
    QCOMPARE(lines.at(1), QStringLiteral("sv_cheats 1 // cheats on"));
    QCOMPARE(lines.at(2), QStringLiteral("volume 0.5"));
    QCOMPARE(lines.size(), 4); // the trailing terminator split leaves an empty piece
}

void EngineTest::trailingCommentEditsThroughTheRawField() {
    const QString path = writeFile(QStringLiteral("trailing.cfg"),
                                   QStringLiteral("sv_cheats 1 // cheats on\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    // The user's raw text is authoritative: the field shows the tail, so an edit that
    // keeps the text keeps the comment, and dropping the text drops it (§6).
    QVERIFY(controller.setRowText(0, QStringLiteral("sv_cheats 0 // cheats off")));
    QVERIFY(controller.saveCfg());
    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(saved.readAll()), QStringLiteral("sv_cheats 0 // cheats off\n"));
    saved.close();

    // The resynced model still carries the tail, so the next edit sees it too.
    QCOMPARE(controller.rowInfo(0).value(QStringLiteral("line")).toString(),
             QStringLiteral("sv_cheats 0 // cheats off"));
    QVERIFY(controller.setRowText(0, QStringLiteral("sv_cheats 1")));
    QVERIFY(controller.saveCfg());
    QFile second(path);
    QVERIFY(second.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(second.readAll()), QStringLiteral("sv_cheats 1\n"));
    second.close();
}

void EngineTest::compoundInsertStaysOneLine() {
    const QString path = writeFile(QStringLiteral("compound.cfg"),
                                   QStringLiteral("volume 0.5\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    // A multi-segment insert is ONE file line: the segments share a groupId (§5).
    const int row = controller.insertRowAfter(-1, QStringLiteral("echo a; echo b"));
    QVERIFY(row >= 0);
    QCOMPARE(controller.rowInfo(row + 1).value(QStringLiteral("typeName")).toString(),
             QStringLiteral("command"));
    QVERIFY(controller.saveCfg());
    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(saved.readAll()),
             QStringLiteral("volume 0.5\necho a; echo b\n"));
    saved.close();

    // Editing any segment row replaces the whole line, not just that segment.
    QVERIFY(controller.setRowText(row, QStringLiteral("echo a2; echo b2")));
    QVERIFY(controller.saveCfg());
    QFile second(path);
    QVERIFY(second.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(second.readAll()),
             QStringLiteral("volume 0.5\necho a2; echo b2\n"));
    second.close();
}

void EngineTest::compoundTailSurvivesAnEdit() {
    const QString path = writeFile(QStringLiteral("compoundtail.cfg"),
                                   QStringLiteral("echo a; echo b // note\n"));

    ConfigController controller;
    LOAD_CONFIG(controller, path);
    // Two rows (one per segment), the tail on both; an edit replaces the whole line
    // and the new tail rides on the last segment's text.
    QVERIFY(controller.setRowText(0, QStringLiteral("echo a2; echo b2 // new")));
    QVERIFY(controller.saveCfg());
    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(saved.readAll()),
             QStringLiteral("echo a2; echo b2 // new\n"));
    saved.close();
    // The Overview line shows the tail (LineRole), so nothing turns invisible.
    QVERIFY(controller.rowInfo(1).value(QStringLiteral("line")).toString()
                .contains(QLatin1String("// new")));
}

QTEST_MAIN(EngineTest)
#include "tst_engine.moc"
