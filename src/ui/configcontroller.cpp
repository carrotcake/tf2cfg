#include "configcontroller.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMultiHash>
#include <QSaveFile>
#include <QSet>
#include <QVariantMap>
#include <QtConcurrent>

#include <algorithm>

ConfigController::ConfigController(QObject *parent)
    : QObject(parent) {
    // GameCfg's default constructor already ran loadAllCmds(); calling it again would
    // duplicate every known command, so it is deliberately not repeated here.

    // The bind map belongs to the model (it owns the rows); QML sees it as cfg.binds.
    connect(&m_commands, &CommandListModel::bindsChanged, this, &ConfigController::bindsChanged);

    // Build-A-Bind browses the game's command catalog, not the parsed config.
    m_console.setCommands(m_cfg.allCmds());
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
 * are inserted after the last bind line, in row order, or at end of file when the
 * config has no binds at all.
 *
 * shortcut: several commands on one file line (`;`) are re-rendered as several lines,
 * and a deleted row among several on one line keeps the whole line. Upgrade when a
 * real config trips over it.
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

    // ownedByLine[i] = rows the app owns on file line i (several when the line used
    // `;`), each re-rendered on its own output line. modelLines is every file line the
    // model still holds a row for; a line the parse covered (parsedLines, the load-time
    // snapshot) but the model no longer does was deleted by the user (clearBind), so it
    // is dropped instead of copied verbatim.
    QMultiHash<int, int> ownedByLine;
    QSet<int> modelLines;
    QSet<int> parsedLines;
    for (const cmd::Entry &parsed : m_parsed)
        if (parsed.sourceLine >= 0)
            parsedLines.insert(parsed.sourceLine);
    int insertAfter = -1; // last bind line: session rows land right after the binds
    for (int row = 0; row < entries.size(); ++row) {
        const cmd::Entry &entry = entries.at(row);
        if (entry.sourceLine < 0)
            continue;
        modelLines.insert(entry.sourceLine);
        if (entry.appOwned
            || CommandListModel::isBindCommand(entry.args.value(0).toLower()))
            insertAfter = qMax(insertAfter, entry.sourceLine);
        if (entry.appOwned)
            ownedByLine.insert(entry.sourceLine, row);
    }

    QByteArray out;
    out.reserve(bytes.size() + 128);
    qint64 insertOffset = -1; // byte offset in out right after the insertAfter line
    for (int i = 0; i < lines.size(); ++i) {
        const QByteArray lineBytes = lines.at(i);
        QByteArray terminator;
        if (lineBytes.endsWith("\r\n"))
            terminator = QByteArrayLiteral("\r\n");
        else if (lineBytes.endsWith('\n') || lineBytes.endsWith('\r'))
            terminator = QByteArray(1, lineBytes.at(lineBytes.size() - 1));

        if (ownedByLine.contains(i)) {
            QList<int> rows = ownedByLine.values(i);
            std::sort(rows.begin(), rows.end());
            for (const int row : rows) {
                const cmd::Entry &entry = entries.at(row);
                if (entry.args.isEmpty())
                    continue; // owned but emptied: the line is dropped
                // args.join(" ") is the exact text the Command Overview shows, so what
                // the user reviewed is what lands on disk; the line's own terminator
                // is reused so the file's endings survive.
                out += entry.args.join(QLatin1Char(' ')).toUtf8();
                out += terminator;
            }
        } else if (!parsedLines.contains(i) || modelLines.contains(i)) {
            out += lineBytes; // never parsed, or a row still holds it: byte-for-byte
        }
        // else: parsed once, but its row is gone from the model - drop the line.

        if (i == insertAfter)
            insertOffset = out.size();
    }

    // Rows created in this session, right after the last bind line (so new binds stay
    // with the binds) or at end of file when the config has no binds.
    QByteArray sessionBlock;
    QByteArray nl = QByteArrayLiteral("\n");
    if (insertAfter >= 0 && insertAfter < lines.size()) {
        const QByteArray &nb = lines.at(insertAfter);
        if (nb.endsWith("\r\n"))
            nl = QByteArrayLiteral("\r\n");
        else if (nb.endsWith('\n') || nb.endsWith('\r'))
            nl = QByteArray(1, nb.at(nb.size() - 1));
    }
    for (const cmd::Entry &entry : entries) {
        if (!entry.appOwned || entry.sourceLine >= 0 || entry.args.isEmpty())
            continue;
        sessionBlock += entry.args.join(QLatin1Char(' ')).toUtf8() + nl;
    }
    if (!sessionBlock.isEmpty()) {
        // If the anchor line itself had no terminator, the first row would glue onto it.
        if (insertAfter >= 0 && insertAfter < lines.size()) {
            const QByteArray &nb = lines.at(insertAfter);
            if (!nb.endsWith('\n') && !nb.endsWith('\r'))
                sessionBlock.prepend(nl);
        }
        if (insertOffset < 0)
            out += sessionBlock;
        else
            out.insert(insertOffset, sessionBlock);
    }

    return commitWrite(path, out, error);
}

bool ConfigController::commitWrite(const QString &path, const QByteArray &out,
                                   QString *error) {
    // Backup first: the only insurance against a wrong-but-successful write.
    const QFileInfo info(path);
    const QString backup = info.dir().filePath(info.fileName() + QStringLiteral(".bak"));
    if (info.exists()) {
        QFile backupFile(backup);
        if (backupFile.exists())
            backupFile.remove();
        if (!QFile::copy(path, backup)) {
            *error = QStringLiteral("cannot write the backup %1").arg(backup);
            return false;
        }
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

QAbstractItemModel *ConfigController::commandModel() {
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

bool ConfigController::setBind(const QString &key, const QString &command) {
    return m_commands.setBind(key, command);
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

    // Run parsing on a worker thread so the GUI stays responsive.
    m_loadFuture = QtConcurrent::run([this, path]() {
        // Review finding 6: the engine ignores Lexers::Lexer::open()'s result and would
        // silently produce an empty list. Same check, surfaced instead of swallowed.
        // appendCfg accumulates, so a re-load must not stack rows on the previous file's.
        m_cfg.resetParsed();
        const bool ok = m_cfg.appendCfg(path);
        if (ok)
            m_cfg.printCfg(); // parity P12/D7: the widget build dumped the parsed list to stdout

        // Must update the model on the GUI thread.
        QMetaObject::invokeMethod(this, [this, path, ok]() {
            if (!ok) {
                setLastError(QStringLiteral("file could not be read"));
                setLoading(false);
                emit loadedChanged();
                return;
            }
            m_commands.setEntries(m_cfg.getCmdList());
            m_parsed = m_cfg.getCmdList(); // deletion baseline for the surgical save
            setCfgPath(path);
            recordFileStamp(); // baseline for the disk-changed check at save time
            setLastError(QString());
            setLoading(false);
            emit loadedChanged();
        }, Qt::QueuedConnection);
    });
}

void ConfigController::clear() {
    m_commands.clearEntries();
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
