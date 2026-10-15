#include "configcontroller.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMultiHash>
#include <QPointer>
#include <QSaveFile>
#include <QSet>
#include <QVariantMap>
#include <QtConcurrent>

#include "gamecfg/lex/lexers.h"

#include <algorithm>

ConfigController::ConfigController(QObject *parent)
    : QObject(parent) {
    // GameCfg's default constructor already ran loadAllCmds(); calling it again would
    // duplicate every known command, so it is deliberately not repeated here.

    // The bind map belongs to the model (it owns the rows); QML sees it as cfg.binds.
    connect(&m_commands, &CommandListModel::bindsChanged, this, &ConfigController::bindsChanged);

    // Build-A-Bind browses the game's command catalog, not the parsed config.
    m_console.setCommands(m_cfg.allCmds());

    // The Config Editor's list is this filter over the parsed rows.
    m_rowFilter.setSourceModel(&m_commands);
}

void ConfigController::setLoading(bool loading) {
    if (m_loading == loading)
        return;
    m_loading = loading;
    emit loadingChanged();
}

void ConfigController::recordFileStamp() {
    const QFileInfo info(m_cfgPath);
    if (info.exists() && info.isFile()) {
        m_loadedStamp = info.lastModified();
        m_loadedSize = info.size();
    } else {
        m_loadedStamp = QDateTime();
        m_loadedSize = -1;
    }
}

bool ConfigController::canSave() const {
    return loaded() && !m_cfgPath.startsWith(QLatin1Char(':'));
}

QString ConfigController::saveBlockReason() const {
    if (!loaded())
        return QStringLiteral("no config is loaded");
    if (m_cfgPath.startsWith(QLatin1Char(':')))
        return QStringLiteral("the loaded config is a bundled preset; use Save As to pick a real file");
    return QString();
}

bool ConfigController::needsConfirm() const {
    if (!canSave())
        return false;
    const QFileInfo info(m_cfgPath);
    if (!info.exists() || !info.isFile())
        return true; // vanished since load
    if (!info.isReadable() || !info.isWritable())
        return true; // read-only
    return info.lastModified() != m_loadedStamp || info.size() != m_loadedSize;
}

bool ConfigController::isDirty() const {
    // A deleted parsed row also changes the file on the next save (its line is
    // dropped) even though no app-owned row remains (docs/DESIGN-config-editor.md).
    if (m_droppedParsedLines > 0)
        return true;
    for (const cmd::Entry &entry : m_commands.entries())
        if (entry.appOwned)
            return true;
    return false;
}

bool ConfigController::saveCfg() {
    if (!canSave()) {
        setLastError(saveBlockReason());
        return false;
    }
    QString error;
    if (!writeConfig(m_cfgPath, m_cfgPath, &error)) {
        setLastError(error);
        return false;
    }
    setLastError(QString());
    recordFileStamp();
    emit saved(m_cfgPath);
    resyncAfterSave();
    return true;
}

bool ConfigController::saveCfgAs(const QString &path) {
    if (!loaded()) {
        setLastError(QStringLiteral("no config is loaded"));
        return false;
    }
    if (path.isEmpty() || path.startsWith(QLatin1Char(':'))) {
        setLastError(QStringLiteral("pick a writable file path"));
        return false;
    }
    QString error;
    // The target should become the loaded config, so the surgical baseline is the
    // loaded file (a `:/` preset for bundled presets), never the target's old bytes -
    // merging a preset's binds into some unrelated file's content would be a chimera.
    if (!writeConfig(path, m_cfgPath, &error)) {
        setLastError(error);
        return false;
    }
    setCfgPath(path); // a preset saved to a real path becomes that file
    setLastError(QString());
    recordFileStamp();
    emit saved(path);
    resyncAfterSave();
    return true;
}

/**
 * The file just written is the new source of truth: rebuild the rows and the parse
 * snapshot from it, so the next surgical save keys every row to the right line of the
 * NEW layout (a save that inserted session rows or dropped deleted ones shifted the
 * lines), `dirty` goes false until the next edit, and - the point of this request -
 * after Save As the chosen path is the file all later editing and saving works on.
 *
 * Synchronous on purpose: a queued reload could land after the user's next edit and
 * overwrite it. A parsed .cfg is a few hundred lines; the parse is microseconds.
 */
void ConfigController::resyncAfterSave() {
    m_cfg.resetParsed();
    if (!m_cfg.appendCfg(m_cfgPath)) {
        // We just wrote the file successfully; a read failure here is filesystem-level
        // (permissions racing). Report it, but keep the current rows rather than
        // blanking the model over a save the user watched succeed.
        setLastError(QStringLiteral("the saved file could not be re-read"));
        return;
    }
    m_commands.setEntries(m_cfg.getCmdList());
    m_parsed = m_cfg.getCmdList();
    m_droppedParsedLines = 0; // the resync reproduces the saved file exactly
}

/**
 * The surgical save (docs/legacy/ui-qml-rewrite.md 10.1). The file on disk is the
 * source of truth for everything the app does not own; the model is the authority
 * only for its app-owned rows.
 *
 * Lines are split preserving each terminator, so a CRLF config stays CRLF and a file
 * with no trailing newline keeps that property. An app-owned line is re-rendered as
 * `args.join(" ")` — the exact text the Command Overview shows, so there is one
 * rendering, not two. A line the parse covered but whose row the model no longer holds
 * (the user deleted the bind) is dropped. Rows added in this session (`sourceLine < 0`)
 * insert at their anchor (Entry::anchorLine, docs/DESIGN-config-editor.md §9); the
 * legacy anchor -1 keeps the original rule (after the last bind line).
 *
 * shortcut: a parsed-but-deleted command on an untouched compound line keeps the whole
 * line, because nothing owns that line. Touched compound lines now rebuild all of their
 * surviving commands together. Upgrade when a real config trips over it.
 */
bool ConfigController::writeConfig(const QString &path, const QString &source,
                                   QString *error) {
    // The source may not exist (a fresh Save As target, or a file that vanished since
    // load): that is an empty baseline, not an error.
    QByteArray bytes;
    if (QFile::exists(source)) {
        QFile original(source);
        if (!original.open(QIODevice::ReadOnly)) {
            *error = QStringLiteral("cannot read %1").arg(source);
            return false;
        }
        // No QIODevice::Text: it would normalize line endings and rewrite the whole file.
        bytes = original.readAll();
        original.close();
    }

    // Split keeping the terminators, so the join at the end reproduces the file exactly.
    // \r\n is one terminator; a bare \r or \n is one on its own.
    QList<QByteArray> lines;
    {
        int start = 0;
        while (start < bytes.size()) {
            int i = start;
            while (i < bytes.size() && bytes.at(i) != '\n' && bytes.at(i) != '\r')
                ++i;
            if (i == bytes.size()) {
                lines.append(bytes.mid(start)); // final unterminated line
                break;
            }
            const int len = (bytes.at(i) == '\r' && i + 1 < bytes.size()
                                 && bytes.at(i + 1) == '\n')
                                ? 2
                                : 1;
            lines.append(bytes.mid(start, i - start + len));
            start = i + len;
        }
    }

    const QVector<cmd::Entry> &entries = m_commands.entries();

    // Empty or vanished source: there is nothing to preserve, so the model's rows are
    // the whole file. Render every row - the surgical machinery below would drop the
    // unowned parsed ones along with the missing baseline.
    if (lines.isEmpty() && !entries.isEmpty()) {
        QByteArray out;
        for (const cmd::Entry &entry : entries) {
            if (entry.args.isEmpty())
                continue;
            out += entry.args.join(QLatin1Char(' ')).toUtf8();
            out += '\n';
        }
        return commitWrite(path, out, error);
    }

    // rowsByLine groups every surviving row by the source line it was parsed from, so a
    // `;`-compound line is edited and rewritten as one line instead of being rendered
    // in several pieces. Each app-owned row on that line marks the whole line for
    // rebuilding.
    QHash<int, QVector<int>> rowsByLine;
    QSet<int> parsedLines;
    for (const cmd::Entry &parsed : m_parsed)
        if (parsed.sourceLine >= 0)
            parsedLines.insert(parsed.sourceLine);

    // Session rows grouped by their save anchor (docs/DESIGN-config-editor.md §9):
    // anchorLine -1 = the legacy bind rule, cmd::ANCHOR_EOF = end of file, anything
    // else = right after that source line's emitted text.
    QHash<int, QVector<int>> sessionsByAnchor;
    for (int row = 0; row < entries.size(); ++row) {
        const cmd::Entry &entry = entries.at(row);
        if (entry.appOwned && entry.sourceLine < 0 && !entry.args.isEmpty())
            sessionsByAnchor[entry.anchorLine].append(row);
    }
    QSet<int> anchorLines;
    for (auto it = sessionsByAnchor.constBegin(); it != sessionsByAnchor.constEnd(); ++it)
        if (it.key() >= 0 && it.key() != cmd::ANCHOR_EOF)
            anchorLines.insert(it.key());

    int insertAfter = -1; // legacy anchor: last bind line; session binds land after it
    for (int row = 0; row < entries.size(); ++row) {
        const cmd::Entry &entry = entries.at(row);
        if (entry.sourceLine < 0)
            continue;
        rowsByLine[entry.sourceLine].append(row);
        if (entry.appOwned
            || CommandListModel::isBindCommand(entry.args.value(0).toLower()))
            insertAfter = qMax(insertAfter, entry.sourceLine);
    }

    QByteArray out;
    out.reserve(bytes.size() + 128);
    qint64 insertOffset = -1; // byte offset in out right after the insertAfter line
    QHash<int, qint64> anchorOffsets; // explicit anchor line -> offset after its text
    for (int i = 0; i < lines.size(); ++i) {
        const QByteArray lineBytes = lines.at(i);
        QByteArray terminator;
        if (lineBytes.endsWith("\r\n"))
            terminator = QByteArrayLiteral("\r\n");
        else if (lineBytes.endsWith('\n') || lineBytes.endsWith('\r'))
            terminator = QByteArray(1, lineBytes.at(lineBytes.size() - 1));

        auto groupIt = rowsByLine.find(i);
        if (groupIt != rowsByLine.end()) {
            const QVector<int> &rows = groupIt.value();
            bool anyOwned = false;
            for (int row : rows) {
                if (entries.at(row).appOwned) {
                    anyOwned = true;
                    break;
                }
            }
            if (!anyOwned) {
                out += lineBytes; // untouched row: preserve the source line byte-for-byte
            } else {
                // Re-render every surviving command of this source line as one compound
                // line (`; ` between commands), dropping emptied rows. The original
                // semicolon stays on one line instead of being exploded by the writer.
                // The line's comment tail is re-emitted once, after the last surviving
                // command (docs/DESIGN-config-editor.md §6).
                QString rebuilt;
                bool firstCommand = true;
                for (int row : rows) {
                    const cmd::Entry &entry = entries.at(row);
                    if (entry.args.isEmpty())
                        continue; // owned but emptied: that segment drops
                    if (!firstCommand)
                        rebuilt += QStringLiteral("; ");
                    rebuilt += entry.args.join(QLatin1Char(' '));
                    firstCommand = false;
                }
                if (!rebuilt.isEmpty()) {
                    const QString tail = entries.at(rows.last()).commentTail;
                    if (!tail.isEmpty())
                        rebuilt += QLatin1Char(' ') + tail;
                    out += rebuilt.toUtf8();
                    out += terminator;
                }
            }
        } else if (!parsedLines.contains(i)) {
            out += lineBytes; // never parsed: byte-for-byte
        }
        // else: parsed once, but its row is gone from the model - drop the line.

        if (i == insertAfter)
            insertOffset = out.size();
        if (anchorLines.contains(i))
            anchorOffsets[i] = out.size();
    }

    // -- session-row emission ------------------------------------------------------
    // Every group renders to (offset, bytes) and inserts at its offset, descending so
    // earlier inserts never shift a later one. Groups without a usable offset append
    // at end of file (anchor line dropped or never existed).
    struct SessionBlock {
        qint64 offset;
        QByteArray bytes;
    };

    const auto terminatorOf = [&lines](int idx) -> QByteArray {
        if (idx < 0 || idx >= lines.size())
            return QByteArrayLiteral("\n");
        const QByteArray &lineBytes = lines.at(idx);
        if (lineBytes.endsWith("\r\n"))
            return QByteArrayLiteral("\r\n");
        if (lineBytes.endsWith('\n') || lineBytes.endsWith('\r'))
            return QByteArray(1, lineBytes.at(lineBytes.size() - 1));
        return QByteArrayLiteral("\n"); // unterminated last line: rows still separate
    };
    const auto hasOwnTerminator = [&lines](int idx) -> bool {
        if (idx < 0 || idx >= lines.size())
            return true;
        const QByteArray &lineBytes = lines.at(idx);
        return lineBytes.endsWith('\n') || lineBytes.endsWith('\r');
    };
    // Rows of one session LINE: a groupId > 0 set shares one `;`-joined line (a compound
    // insert, docs/DESIGN-config-editor.md §5); groupId 0 rows render individually.
    const auto sessionLinesFor = [&entries](const QVector<int> &rows) {
        QVector<QVector<int>> lines;
        QHash<int, int> indexOfGid;
        for (int row : rows) {
            const int gid = entries.at(row).groupId;
            if (gid > 0 && indexOfGid.contains(gid)) {
                lines[indexOfGid.value(gid)].append(row);
            } else {
                if (gid > 0)
                    indexOfGid.insert(gid, lines.size());
                lines.append(QVector<int>{ row });
            }
        }
        return lines;
    };
    // Rows of one group, each rendered on its own line in row order.
    const auto renderGroup = [&entries, &sessionLinesFor](const QVector<int> &rows,
                                                          const QByteArray &nl) {
        QByteArray block;
        for (const QVector<int> &lineRows : sessionLinesFor(rows)) {
            QString line;
            bool firstSegment = true;
            for (int row : lineRows) {
                const cmd::Entry &entry = entries.at(row);
                if (!entry.appOwned || entry.sourceLine >= 0 || entry.args.isEmpty())
                    continue;
                if (!firstSegment)
                    line += QStringLiteral("; ");
                line += entry.args.join(QLatin1Char(' '));
                firstSegment = false;
            }
            if (line.isEmpty())
                continue; // owned but emptied: that line drops
            block += line.toUtf8() + nl;
        }
        return block;
    };

    QVector<SessionBlock> blocks;
    const auto addBlock = [&blocks, &out](qint64 offset, QByteArray bytes) {
        if (!bytes.isEmpty())
            blocks.append({offset, std::move(bytes)});
    };

    // Legacy binds: after the last bind line, or end of file when there is none.
    if (sessionsByAnchor.contains(-1)) {
        QByteArray nl = terminatorOf(insertAfter);
        QByteArray block = renderGroup(sessionsByAnchor.value(-1), nl);
        // If the anchor line itself had no terminator, the first row would glue onto it.
        if (insertAfter >= 0 && !hasOwnTerminator(insertAfter))
            block.prepend(QByteArrayLiteral("\n"));
        addBlock(insertOffset < 0 ? out.size() : insertOffset, std::move(block));
    }
    // Explicitly anchored rows: right after the anchor line's emitted text.
    for (auto it = sessionsByAnchor.constBegin(); it != sessionsByAnchor.constEnd(); ++it) {
        if (it.key() < 0 || it.key() == cmd::ANCHOR_EOF || !anchorOffsets.contains(it.key()))
            continue;
        QByteArray block = renderGroup(it.value(), terminatorOf(it.key()));
        if (!hasOwnTerminator(it.key()))
            block.prepend(QByteArrayLiteral("\n"));
        addBlock(anchorOffsets.value(it.key()), std::move(block));
    }
    // Unanchored rows: end of file, using the file's own line-ending convention.
    if (sessionsByAnchor.contains(cmd::ANCHOR_EOF)) {
        addBlock(out.size(),
                 renderGroup(sessionsByAnchor.value(cmd::ANCHOR_EOF),
                             terminatorOf(lines.size() - 1)));
    }

    std::stable_sort(blocks.begin(), blocks.end(),
                     [](const SessionBlock &a, const SessionBlock &b) {
                         return a.offset > b.offset;
                     });
    for (const SessionBlock &block : blocks)
        out.insert(static_cast<int>(block.offset), block.bytes);

    return commitWrite(path, out, error);
}

bool ConfigController::commitWrite(const QString &path, const QByteArray &out,
                                   QString *error) {
    // Backup first: the only insurance against a wrong-but-successful write.
    const QFileInfo info(path);
    const QString backup = info.dir().filePath(info.fileName() + QStringLiteral(".bak"));
    if (info.exists()) {
        // Keep the old restore point intact until the new backup is fully written:
        // only the cheap rename phase touches the existing `.bak`.
        const QString newBackup = backup + QStringLiteral(".new");
        const QString oldBackup = backup + QStringLiteral(".old");
        QFile::remove(newBackup);
        QFile::remove(oldBackup);

        if (!QFile::copy(path, newBackup)) {
            *error = QStringLiteral("cannot write the backup %1").arg(backup);
            QFile::remove(newBackup);
            return false;
        }

        const bool hadBackup = QFile::exists(backup);
        if (hadBackup && !QFile::rename(backup, oldBackup)) {
            *error = QStringLiteral("cannot write the backup %1").arg(backup);
            QFile::remove(newBackup);
            return false;
        }
        if (!QFile::rename(newBackup, backup)) {
            if (hadBackup)
                QFile::rename(oldBackup, backup);
            *error = QStringLiteral("cannot write the backup %1").arg(backup);
            return false;
        }

        QFile::remove(oldBackup);
    }

    QSaveFile out2(path);
    if (!out2.open(QIODevice::WriteOnly)) {
        *error = QStringLiteral("cannot write %1").arg(path);
        return false;
    }
    if (out2.write(out) != out.size()) {
        *error = QStringLiteral("cannot write %1").arg(path);
        out2.cancelWriting();
        return false;
    }
    if (!out2.commit()) {
        *error = QStringLiteral("cannot write %1").arg(path);
        return false;
    }
    return true;
}

CommandListModel *ConfigController::commandModel() {
    return &m_commands;
}

QVariantList ConfigController::presets() const {
    return {
        QVariantMap {
            { QStringLiteral("label"), QStringLiteral("b4nny (config.cfg from b4nny.zip)") },
            { QStringLiteral("path"), QStringLiteral(":/cfg/b4nny.cfg") },
            { QStringLiteral("note"), QStringLiteral("Pro-player config: crosshair, viewmodel and net settings") },
        },
        QVariantMap {
            { QStringLiteral("label"), QStringLiteral("b4nny (default binds only)") },
            { QStringLiteral("path"), QStringLiteral(":/cfg/b4nny_defbinds.cfg") },
            { QStringLiteral("note"), QStringLiteral("b4nny's settings + stock default binds") },
        },
        QVariantMap {
            { QStringLiteral("label"), QStringLiteral("b4nny (no binds)") },
            { QStringLiteral("path"), QStringLiteral(":/cfg/b4nny_nobinds.cfg") },
            { QStringLiteral("note"), QStringLiteral("b4nny's settings only, zero binds") },
        },
        QVariantMap {
            { QStringLiteral("label"), QStringLiteral("Stock TF2 config.cfg") },
            { QStringLiteral("path"), QStringLiteral(":/cfg/default.cfg") },
            { QStringLiteral("note"), QStringLiteral("Valve's default config and binds") },
        },
    };
}

QString ConfigController::presetAt(int index) const {
    const QVariantList list = presets();
    if (index < 0 || index >= list.size())
        return QString();
    return list.at(index).toMap().value(QStringLiteral("path")).toString();
}

QString ConfigController::presetNote(int index) const {
    const QVariantList list = presets();
    if (index < 0 || index >= list.size())
        return QString();
    return list.at(index).toMap().value(QStringLiteral("note")).toString();
}

QString ConfigController::cfgPath() const {
    return m_cfgPath;
}

QString ConfigController::lastError() const {
    return m_lastError;
}

bool ConfigController::loaded() const {
    return m_commands.rowCount() > 0;
}

QVariantMap ConfigController::binds() const {
    return m_commands.binds();
}

QVariantMap ConfigController::bindingKinds() const {
    return m_commands.bindingKinds();
}

QString ConfigController::bindingKind(const QString &key) const {
    return m_commands.bindingKind(key);
}

QVariantMap ConfigController::parseCommandLine(const QString &line) const {
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);

    Lexers::cfgLex lex;
    lex.setBuffer({line});
    QStringList args;
    while (lex.hasNext()) {
        const auto tok = lex.nextToken();
        if (tok.isEndToken())
            break;
        args << tok.string();
    }
    if (args.isEmpty()) {
        result.insert(QStringLiteral("error"), QStringLiteral("nothing to parse"));
        return result;
    }

    const cmd::CmdType type = m_cfg.lookupType(args.first());
    QVariantList argList;
    for (const QString &arg : args)
        argList << arg;
    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("args"), argList);
    result.insert(QStringLiteral("type"), static_cast<int>(type));
    result.insert(QStringLiteral("typeName"), CommandListModel::typeName(type));
    // Trailing comment (`// ...` raw text) the line carried; empty when none (§6).
    result.insert(QStringLiteral("tail"), lex.tailAt(0));
    return result;
}

bool ConfigController::setRowText(int row, const QString &text) {
    if (row < 0 || row >= m_commands.entries().size())
        return false;
    const QString trimmed = text.trimmed();

    // Comment rows: the raw text IS the content (§6) - no tokenizing, and a missing
    // `//` is restored so the row still reads back as a comment after a resync.
    if (m_commands.entries().at(row).type == cmd::CMD_COMMENT) {
        if (trimmed.isEmpty())
            return false;
        cmd::Entry comment;
        comment.args << (trimmed.startsWith(QLatin1String("//")) ? trimmed
                                                                 : QStringLiteral("// ")
                                                                       + trimmed);
        comment.type = cmd::CMD_COMMENT;
        return m_commands.setRowGroup(row, {comment});
    }

    // The text may be a compound line (`a; b`): split on unquoted `;` and parse each
    // segment, so an edit of one row replaces the WHOLE line the row belongs to (§5).
    QVector<cmd::Entry> segments;
    for (const QString &segment : splitSegments(trimmed)) {
        const QVariantMap parsed = parseCommandLine(segment);
        if (!parsed.value(QStringLiteral("ok")).toBool())
            return false;
        cmd::Entry entry;
        for (const QVariant &arg : parsed.value(QStringLiteral("args")).toList())
            entry.args << arg.toString();
        entry.type =
            static_cast<cmd::CmdType>(parsed.value(QStringLiteral("type")).toInt());
        entry.commentTail = parsed.value(QStringLiteral("tail")).toString();
        segments << entry;
    }
    return m_commands.setRowGroup(row, segments);
}

bool ConfigController::deleteRow(int row) {
    // Pending file change without an app-owned row: track it for dirty (see isDirty).
    if (row >= 0 && row < m_commands.entries().size()
        && m_commands.entries().at(row).sourceLine >= 0)
        ++m_droppedParsedLines;
    return m_commands.deleteEntry(row);
}

int ConfigController::insertRowAfter(int anchorRow, const QString &text) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return -1;

    // A `//`-prefixed insert is a comment line: raw text, no tokenizing (§6).
    QVector<cmd::Entry> segments;
    if (trimmed.startsWith(QLatin1String("//"))) {
        cmd::Entry comment;
        comment.args << trimmed;
        comment.type = cmd::CMD_COMMENT;
        segments << comment;
    } else {
        for (const QString &segment : splitSegments(trimmed)) {
            const QVariantMap parsed = parseCommandLine(segment);
            if (!parsed.value(QStringLiteral("ok")).toBool())
                return -1;
            cmd::Entry entry;
            for (const QVariant &arg : parsed.value(QStringLiteral("args")).toList())
                entry.args << arg.toString();
            entry.type =
                static_cast<cmd::CmdType>(parsed.value(QStringLiteral("type")).toInt());
            entry.commentTail = parsed.value(QStringLiteral("tail")).toString();
            segments << entry;
        }
    }
    return m_commands.insertLine(anchorRow, segments);
}

bool ConfigController::setAlias(const QString &name, const QString &body) {
    // A removed definition drops its file line on the next save without leaving an
    // app-owned row behind, so it counts as a pending deletion for dirty (isDirty).
    const bool existed = m_commands.aliasInfo(name)
                             .value(QStringLiteral("found")).toBool();
    const bool ok = m_commands.setAlias(name, body);
    if (ok && existed && body.trimmed().isEmpty())
        ++m_droppedParsedLines;
    return ok;
}

QStringList ConfigController::aliasNames() const {
    return m_commands.aliasNames();
}

QVariantMap ConfigController::aliasInfo(const QString &name) const {
    return m_commands.aliasInfo(name);
}

QStringList ConfigController::splitSegments(const QString &line) const {
    // Quote- and escape-aware on the lexer's terms: `;` separates only outside
    // quotes, and inside quotes a backslash escapes the next character (so `\"`
    // never opens or closes a quote). The pieces stay raw text - the segment
    // editors show exactly what the user wrote.
    QStringList out;
    QString current;
    bool quoted = false;
    bool escaped = false;
    for (const QChar c : line) {
        if (escaped) {
            current += c;
            escaped = false;
        } else if (quoted && c == QLatin1Char('\\')) {
            current += c;
            escaped = true;
        } else if (c == QLatin1Char('"')) {
            quoted = !quoted;
            current += c;
        } else if (c == QLatin1Char(';') && !quoted) {
            out << current.trimmed();
            current.clear();
        } else {
            current += c;
        }
    }
    out << current.trimmed();
    // Stray separators leave empty pieces; they carry nothing and drop everywhere.
    out.removeAll(QString());
    return out;
}

QList<QVariant> ConfigController::parseSignature(const QString &signature) const
{
    const QVector<cmd::SignatureArg> args = cmd::parseSignature(signature);
    QVariantList out;
    out.reserve(args.size());
    for (const cmd::SignatureArg &arg : args) {
        out.append(QVariantMap { { QStringLiteral("kind"), arg.kind },
                                 { QStringLiteral("optional"), arg.optional } });
    }
    return out;
}

QString ConfigController::quoteToken(const QString &raw) const {
    return CommandListModel::quoted(raw);
}

QString ConfigController::unquoteToken(const QString &token) const {
    return CommandListModel::unquoted(token);
}

bool ConfigController::setBind(const QString &key, const QString &command) {
    return m_commands.setBind(key, command);
}

bool ConfigController::setBindToggle(const QString &key, const QString &command) {
    return m_commands.setBindToggle(key, command);
}

bool ConfigController::clearBind(const QString &key) {
    return m_commands.clearBind(key);
}

void ConfigController::loadCfg(const QString &path) {
    if (path.isEmpty()) {
        setLastError(QStringLiteral("no config selected"));
        emit loadedChanged();
        return;
    }

    // Review finding 6: the engine ignores Lexers::Lexer::open()'s result and would
    // silently produce an empty list. Same check, surfaced instead of swallowed.
    if (!QFile::exists(path)) {
        setLastError(QStringLiteral("file not found"));
        emit loadedChanged();
        return;
    }

    if (m_loadFuture.isRunning()) {
        // A load is already in flight: ignore the request instead of parsing a
        // second file over the same GameCfg on another thread (docs/AUDIT.md #2).
        return;
    }

    setLoading(true);
    setLastError(QString());

    // Keep a weak reference so a load that outlives the controller (app shutdown) cannot
    // run its queued completion on a destroyed object (docs/AUDIT.md #3).
    QPointer<ConfigController> self = this;

    // Run parsing on a worker thread so the GUI stays responsive.
    m_loadFuture = QtConcurrent::run([path, self]() {
        if (!self)
            return;

        // Review finding 6: the engine ignores Lexers::Lexer::open()'s result and would
        // silently produce an empty list. Same check, surfaced instead of swallowed.
        // appendCfg accumulates, so a re-load must not stack rows on the previous file's.
        self->m_cfg.resetParsed();
        const bool ok = self->m_cfg.appendCfg(path);
        if (ok && self)
            self->m_cfg.printCfg(); // parity P12/D7: the widget build dumped the parsed list to stdout

        if (!self)
            return;

        // Must update the model on the GUI thread.
        QMetaObject::invokeMethod(self, [self, path, ok]() {
            if (!self)
                return;
            if (!ok) {
                self->setLastError(QStringLiteral("file could not be read"));
                self->setLoading(false);
                emit self->loadedChanged();
                return;
            }
            self->m_commands.setEntries(self->m_cfg.getCmdList());
            self->m_parsed = self->m_cfg.getCmdList(); // deletion baseline for the surgical save
            self->setCfgPath(path);
            self->recordFileStamp(); // baseline for the disk-changed check at save time
            self->setLastError(QString());
            self->setLoading(false);
            emit self->loadedChanged();
        }, Qt::QueuedConnection);
    });
}

void ConfigController::clear() {
    m_commands.clearEntries();
    m_droppedParsedLines = 0;
    setCfgPath(QString());
    setLastError(QString());
    emit loadedChanged();
}

void ConfigController::setCfgPath(const QString &path) {
    if (m_cfgPath == path)
        return;
    m_cfgPath = path;
    emit cfgPathChanged();
}

void ConfigController::setLastError(const QString &error) {
    if (m_lastError == error)
        return;
    m_lastError = error;
    emit lastErrorChanged();
}
