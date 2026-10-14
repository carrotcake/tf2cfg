#include "commandlistmodel.h"

#include <QLatin1StringView>

namespace {

QHash<int, QByteArray> makeRoleNames() {
    QHash<int, QByteArray> names;
    names[CommandListModel::LineRole] = "line";
    names[CommandListModel::CommandRole] = "command";
    names[CommandListModel::ArgsRole] = "args";
    names[CommandListModel::TypeRole] = "cmdType";
    names[CommandListModel::TypeNameRole] = "typeName";
    names[CommandListModel::BoundKeyRole] = "boundKey";
    names[CommandListModel::WarningRole] = "warning";
    names[CommandListModel::RowRole] = "sourceRow";
    return names;
}

/// The config lexer hands arguments back with their quotes still on, so `bind "0" "slot10"`
/// parses as three arguments: `bind`, `"0"` and `"slot10"`. Both directions live as
/// public statics on CommandListModel (the Config Editor's chain editor and the alias
/// API re-quote bodies through them): read a key/command out of a parsed row, and write
/// a row the parser would reparse the same way. See docs/legacy/ui-qml-rewrite.md D19.
/// The console commands that drive key binds. scanBinds() reads them and setBind()
/// writes one, so a typo in a copy would produce a row nothing ever reads back.
/// All lowercase: the engine folds every config line to lower case (allcmds.csv
/// spells `bindtoggle` BindToggle). Private to this file - no other translation
/// unit needs them.
namespace BindSyntax {
constexpr auto Bind = QLatin1StringView("bind");
constexpr auto BindToggle = QLatin1StringView("bindtoggle");
constexpr auto Unbind = QLatin1StringView("unbind");
constexpr auto UnbindAll = QLatin1StringView("unbindall");
} // namespace BindSyntax

} // namespace

/// True for the two commands that put a command on a key. Declared static on the
/// model because the surgical save needs the same test to place new bind rows.
bool CommandListModel::isBindCommand(const QString &head) {
    return head == BindSyntax::Bind || head == BindSyntax::BindToggle;
}

QString CommandListModel::unquoted(const QString &arg) {
    QString body = arg;
    if (body.size() >= 2 && body.startsWith(QLatin1Char('"')) && body.endsWith(QLatin1Char('"')))
        body = body.mid(1, body.size() - 2);

    QString out;
    for (int i = 0; i < body.size(); ++i) {
        QChar c = body.at(i);
        // \" and \\ stand for one character; a backslash in front of anything else is
        // ordinary text and is kept.
        if (c == QLatin1Char('\\') && i + 1 < body.size()
            && (body.at(i + 1) == QLatin1Char('"') || body.at(i + 1) == QLatin1Char('\\')))
            c = body.at(++i);
        out.append(c);
    }
    return out;
}

QString CommandListModel::quoted(const QString &arg) {
    QString out;
    for (const QChar c : arg) {
        if (c == QLatin1Char('"') || c == QLatin1Char('\\'))
            out.append(QLatin1Char('\\'));
        out.append(c);
    }
    return QLatin1Char('"') + out + QLatin1Char('"');
}

CommandListModel::CommandListModel(QObject *parent)
    : QAbstractListModel(parent) {}

int CommandListModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_entries.size());
}

QVariant CommandListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};

    const cmd::Entry &entry = m_entries.at(index.row());

    switch (role) {
    case LineRole:
        // The line as the user sees it: the command tokens plus the row's comment
        // tail, exactly what an owned save re-renders (docs/DESIGN-config-editor.md §6).
        return entry.commentTail.isEmpty() ? entry.args.join(" ")
                                           : entry.args.join(" ") + " " + entry.commentTail;
    case CommandRole:
        return entry.args.value(0);
    case ArgsRole:
        return entry.args.mid(1);
    case TypeRole:
        return static_cast<int>(entry.type);
    case TypeNameRole:
        return typeName(entry.type);
    case BoundKeyRole: {
        return boundKeyOf(entry);
    }
    case WarningRole:
        ensureScan();
        return m_warningsCache.value(index.row(), QString());
    case RowRole:
        return index.row();
    default:
        return {};
    }
}

QHash<int, QByteArray> CommandListModel::roleNames() const {
    static const QHash<int, QByteArray> names = makeRoleNames();
    return names;
}

void CommandListModel::setEntries(const QVector<cmd::Entry> &entries) {
    beginResetModel();
    m_entries = entries;
    m_nextGroupId = 1; // the parse re-derives every line identity
    endResetModel();
    invalidateDerived();
}

void CommandListModel::clearEntries() {
    beginResetModel();
    m_entries.clear();
    m_nextGroupId = 1;
    endResetModel();
    invalidateDerived();
}

QVariantMap CommandListModel::binds() const {
    ensureScan();
    return m_bindsCache;
}

QVariantMap CommandListModel::bindingKinds() const {
    ensureScan();
    return m_bindingKindsCache;
}

QString CommandListModel::bindingKind(const QString &key) const {
    const QVariantMap kinds = bindingKinds();
    const QString cleanKey = unquoted(key).toLower();
    return kinds.value(cleanKey).toString();
}

QVector<int> CommandListModel::bindRows(const QString &key) const {
    const BindScanResult result = scanBinds();
    const QString wanted = unquoted(key).toLower();
    return result.rowsByKey.value(wanted);
}

QString CommandListModel::boundKeyOf(const cmd::Entry &entry) const {
    if (entry.args.isEmpty())
        return QString();
    const QString head = unquoted(entry.args.at(0)).toLower();
    if (!isBindCommand(head) || entry.args.size() < 2)
        return QString();
    return unquoted(entry.args.at(1)).toLower();
}

CommandListModel::BindScanResult CommandListModel::scanBinds() const {
    BindScanResult result;

    // Replay-order warning bookkeeping (docs/DESIGN-config-editor.md §7): the last
    // bind/bindtoggle row per key (an unbind after it kills it), every bind row so far
    // (an unbindall after them kills them all), and the rows that set each cvar head
    // (a later set leaves the earlier ones dead). Line numbers in the text are 1-based.
    QHash<QString, int> lastBindRow;
    QVector<int> bindRowsSoFar;
    QHash<QString, QVector<int>> cvarSets;
    const auto deadBy = [&result](int row, const QString &what) {
        result.warnings.insert(row, QStringLiteral("dead: %1").arg(what));
    };

    for (int row = 0; row < m_entries.size(); ++row) {
        const cmd::Entry &entry = m_entries.at(row);
        if (entry.args.isEmpty())
            continue;

        const QString head = unquoted(entry.args.at(0)).toLower();

        if (head == BindSyntax::UnbindAll) {
            for (int earlier : bindRowsSoFar)
                deadBy(earlier, QStringLiteral("unbindall on line %1 clears this bind")
                                     .arg(entry.sourceLine + 1));
            bindRowsSoFar.clear();
            lastBindRow.clear();
            result.binds.clear();
            result.bindingKinds.clear();
            result.rowsByKey.clear();
            continue;
        }
        if (head == BindSyntax::Unbind) {
            if (entry.args.size() >= 2) {
                const QString key = unquoted(entry.args.at(1)).toLower();
                if (lastBindRow.contains(key)) {
                    deadBy(lastBindRow.value(key),
                           QStringLiteral("unbind on line %1 clears this bind")
                               .arg(entry.sourceLine + 1));
                    lastBindRow.remove(key);
                }
                result.binds.remove(key);
                result.bindingKinds.remove(key);
                result.rowsByKey[key].append(row);
            }
            continue;
        }
        if (!isBindCommand(head) || entry.args.size() < 2)
            continue;

        const QString key = unquoted(entry.args.at(1)).toLower();
        if (key.isEmpty())
            continue;

        // `bind "r" ""` is how a config unbinds a key.
        const QString command = entry.args.size() >= 3 ? unquoted(entry.args.at(2)) : QString();
        result.rowsByKey[key].append(row);
        if (command.isEmpty()) {
            result.binds.remove(key);
            result.bindingKinds.remove(key);
        } else {
            result.binds.insert(key, command);
            result.bindingKinds.insert(key, head == BindSyntax::BindToggle
                                               ? QStringLiteral("bindtoggle")
                                               : QStringLiteral("bind"));
            lastBindRow.insert(key, row);
            bindRowsSoFar.append(row);
        }
    }

    // Cvar sets, second pass: flag every earlier row a later set overrides. Plain
    // value rows only - commands and aliases have no "last write wins" death.
    for (int row = 0; row < m_entries.size(); ++row) {
        const cmd::Entry &entry = m_entries.at(row);
        if (entry.args.isEmpty())
            continue;
        switch (entry.type) {
        case cmd::CMD_IVAR:
        case cmd::CMD_FVAR:
        case cmd::CMD_SVAR:
        case cmd::CMD_BVAR: {
            const QString head = unquoted(entry.args.at(0)).toLower();
            for (int earlier : cvarSets.value(head)) {
                if (!result.warnings.contains(earlier))
                    deadBy(earlier, QStringLiteral("overridden by the line %1 set")
                                        .arg(entry.sourceLine + 1));
            }
            cvarSets[head].append(row);
            break;
        }
        default:
            break;
        }
    }

    return result;
}

void CommandListModel::ensureScan() const {
    if (m_bindsCacheValid && m_bindingKindsCacheValid && m_warningsCacheValid)
        return;
    const BindScanResult result = scanBinds();
    m_bindsCache = result.binds;
    m_bindingKindsCache = result.bindingKinds;
    m_warningsCache = result.warnings;
    m_bindsCacheValid = true;
    m_bindingKindsCacheValid = true;
    m_warningsCacheValid = true;
}

void CommandListModel::invalidateDerived() {
    m_bindsCacheValid = false;
    m_bindingKindsCacheValid = false;
    m_warningsCacheValid = false;
    if (!m_entries.isEmpty())
        emit dataChanged(index(0), index(m_entries.size() - 1), {WarningRole});
    emit bindsChanged();
}

bool CommandListModel::setBind(const QString &key, const QString &command, bool toggle) {
    const QString verb = toggle ? QString(BindSyntax::BindToggle) : QString(BindSyntax::Bind);
    const QString cleanKey = unquoted(key).toLower();
    // The command name is case-insensitive, its arguments are not: `say "Hello"` must not
    // become `say "hello"`. Rows read from a file are all lowercase anyway, because cfgLex
    // folds every line, so this only ever preserves what the user has just typed.
    const QString trimmed = command.trimmed();
    int nameEnd = 0;
    while (nameEnd < trimmed.size() && !trimmed.at(nameEnd).isSpace())
        ++nameEnd;
    const QString cleanCommand = trimmed.left(nameEnd).toLower() + trimmed.mid(nameEnd);

    if (cleanKey.isEmpty())
        return false;
    if (cleanCommand.isEmpty())
        return clearBind(cleanKey);
    if (binds().value(cleanKey).toString() == cleanCommand
        && bindingKinds().value(cleanKey).toString() == verb)
        return false; // already bound to exactly this

    const QVector<int> rows = bindRows(cleanKey);
    // Rebind the key's most recent row of the same kind in place when it exists, so
    // the row keeps its sourceLine and the surgical save rewrites the line where it is
    // instead of moving the bind to the end of the file (10.1: an edit keeps its
    // position). Rows of the other kind and unbind rows are reviewed below.
    for (int i = rows.size() - 1; i >= 0; --i) {
        const int row = rows.at(i);
        const QString head = unquoted(m_entries.at(row).args.value(0)).toLower();
        if (head != verb)
            continue; // other kinds are replaced below instead of patched in place
        // A later unbind/bindtoggle row for the same key would replay after this one
        // and override the new command (audit docs/AUDIT.md #1), so drop them too; the
        // append path already drops every row for the key.
        for (int j = rows.size() - 1; j > i; --j) {
            const int laterRow = rows.at(j);
            beginRemoveRows(QModelIndex(), laterRow, laterRow);
            m_entries.remove(laterRow);
            endRemoveRows();
        }
        cmd::Entry &entry = m_entries[row];
        entry.appOwned = true; // now the app owns this line
        entry.args.clear();
        entry.args << verb << quoted(cleanKey) << quoted(cleanCommand);
        entry.type = cmd::CMD_CMD;
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx, {LineRole, CommandRole, ArgsRole, TypeRole, TypeNameRole,
                                    BoundKeyRole});
        invalidateDerived();
        return true;
    }

    for (int i = rows.size() - 1; i >= 0; --i) {
        const int row = rows.at(i);
        beginRemoveRows(QModelIndex(), row, row);
        m_entries.remove(row);
        endRemoveRows();
    }

    cmd::Entry entry;
    // The row is app-owned: the surgical save re-renders exactly these rows and leaves
    // every parsed line byte-for-byte (docs/legacy/ui-qml-rewrite.md 10.1). sourceLine
    // stays -1, which is what puts it after the last owned line at save time.
    entry.appOwned = true;
    entry.args << verb << quoted(cleanKey) << quoted(cleanCommand);
    // `bind` is a plain console command in allcmds.csv, which is the type the parser gives
    // an identical line in a config file. Keep the two indistinguishable.
    entry.type = cmd::CMD_CMD;

    const int row = m_entries.size();
    beginInsertRows(QModelIndex(), row, row);
    m_entries.push_back(entry);
    endInsertRows();

    invalidateDerived();
    return true;
}

bool CommandListModel::setBindToggle(const QString &key, const QString &command) {
    return setBind(key, command, true);
}

bool CommandListModel::clearBind(const QString &key) {
    const QVector<int> rows = bindRows(key);
    if (rows.isEmpty())
        return false;

    for (int i = rows.size() - 1; i >= 0; --i) {
        const int row = rows.at(i);
        beginRemoveRows(QModelIndex(), row, row);
        m_entries.remove(row);
        endRemoveRows();
    }

    invalidateDerived();
    return true;
}

bool CommandListModel::setRowGroup(int row, const QVector<cmd::Entry> &segments) {
    if (row < 0 || row >= m_entries.size() || segments.isEmpty())
        return false;

    // The line-group of `row`: parsed rows group by sourceLine, session compound rows
    // by groupId, everything else stands alone (docs/DESIGN-config-editor.md §5/§6).
    const int srcLine = m_entries.at(row).sourceLine;
    const int gid = m_entries.at(row).groupId;
    int first = row;
    int last = row;
    if (srcLine >= 0) {
        while (first > 0 && m_entries.at(first - 1).sourceLine == srcLine)
            --first;
        while (last + 1 < m_entries.size() && m_entries.at(last + 1).sourceLine == srcLine)
            ++last;
    } else if (gid > 0) {
        while (first > 0 && m_entries.at(first - 1).groupId == gid)
            --first;
        while (last + 1 < m_entries.size() && m_entries.at(last + 1).groupId == gid)
            ++last;
    }

    QVector<cmd::Entry> fresh = segments;
    // Session rows keep their group identity; a lone session row expanded into a
    // compound gets a fresh groupId so the save renders it as one line.
    int groupTag = gid;
    if (srcLine < 0 && fresh.size() > 1 && groupTag == 0)
        groupTag = m_nextGroupId++;
    for (cmd::Entry &entry : fresh) {
        entry.appOwned = true;
        entry.sourceLine = srcLine;
        entry.anchorLine = m_entries.at(row).anchorLine;
        entry.groupId = srcLine < 0 ? groupTag : 0;
    }

    const int groupSize = last - first + 1;
    const int overlap = qMin(groupSize, fresh.size());
    for (int i = 0; i < overlap; ++i)
        m_entries[first + i] = fresh.at(i);

    if (fresh.size() > groupSize) {
        // Extra segments join the same line: insert them as group members.
        beginInsertRows(QModelIndex(), first + groupSize, first + fresh.size() - 1);
        for (int i = groupSize; i < fresh.size(); ++i)
            m_entries.insert(first + i, fresh.at(i));
        endInsertRows();
    } else if (groupSize > fresh.size()) {
        // Extra model rows drop; the save drops their line content with them.
        beginRemoveRows(QModelIndex(), first + fresh.size(), last);
        for (int r = last; r >= first + fresh.size(); --r)
            m_entries.remove(r);
        endRemoveRows();
    }

    emit dataChanged(index(first), index(first + overlap - 1),
                     {LineRole, CommandRole, ArgsRole, TypeRole, TypeNameRole, BoundKeyRole});
    invalidateDerived();
    return true;
}

int CommandListModel::insertLine(int anchorRow, const QVector<cmd::Entry> &segments) {
    if (segments.isEmpty())
        return -1;

    // Save position: right after the anchor row's file line, so the new line lands
    // where the user placed it (docs/DESIGN-config-editor.md §9). An anchor that is
    // itself a session row chains to that row's anchor; no anchor means end of file.
    int anchorLine = cmd::ANCHOR_EOF;
    if (anchorRow >= 0 && anchorRow < m_entries.size()) {
        const cmd::Entry &src = m_entries.at(anchorRow);
        anchorLine = src.sourceLine >= 0 ? src.sourceLine
                   : src.anchorLine >= 0 ? src.anchorLine
                                         : cmd::ANCHOR_EOF;
    }

    QVector<cmd::Entry> fresh = segments;
    const int groupTag = fresh.size() > 1 ? m_nextGroupId++ : 0;
    for (cmd::Entry &entry : fresh) {
        entry.appOwned = true;
        entry.anchorLine = anchorLine;
        entry.groupId = groupTag;
    }

    // Display order mirrors the anchor: the line lands directly after the anchor row,
    // so the list shows it where the save will write it.
    const int row = (anchorRow >= 0 && anchorRow < m_entries.size()) ? anchorRow + 1
                                                                     : m_entries.size();
    beginInsertRows(QModelIndex(), row, row + fresh.size() - 1);
    for (int i = 0; i < fresh.size(); ++i)
        m_entries.insert(row + i, fresh.at(i));
    endInsertRows();
    invalidateDerived();
    return row;
}

bool CommandListModel::deleteEntry(int row) {
    if (row < 0 || row >= m_entries.size())
        return false;

    beginRemoveRows(QModelIndex(), row, row);
    m_entries.remove(row);
    endRemoveRows();
    invalidateDerived();
    return true;
}

// --- alias API (docs/DESIGN-config-editor.md §5.3) ---

namespace {
/// A row that DEFINES an alias (`alias <name> ...`). Rows merely referencing an
/// alias name parse as CMD_ALIAS and never match this test.
bool isAliasDefinition(const cmd::Entry &entry) {
    return !entry.args.isEmpty()
           && CommandListModel::unquoted(entry.args.at(0)).toLower()
                  == QLatin1String("alias");
}

/// The raw body text of an alias definition row: the tokens after the name,
/// unquoted and re-joined. Empty for a bare `alias <name>` row.
QString aliasBodyOf(const cmd::Entry &entry) {
    if (entry.args.size() < 3)
        return QString();
    QStringList parts;
    for (int i = 2; i < entry.args.size(); ++i)
        parts << CommandListModel::unquoted(entry.args.at(i));
    return parts.join(QLatin1Char(' '));
}
} // namespace

bool CommandListModel::setAlias(const QString &name, const QString &body) {
    const QString cleanName = unquoted(name);
    if (cleanName.isEmpty())
        return false;
    const QString cleanBody = body.trimmed();

    QVector<int> rows;
    for (int row = 0; row < m_entries.size(); ++row) {
        const cmd::Entry &entry = m_entries.at(row);
        if (isAliasDefinition(entry)
            && unquoted(entry.args.value(1)).toLower() == cleanName.toLower())
            rows.append(row);
    }

    // An empty body removes the definition - the symmetric counterpart of clearBind.
    if (cleanBody.isEmpty()) {
        if (rows.isEmpty())
            return false;
        for (int i = rows.size() - 1; i >= 0; --i) {
            beginRemoveRows(QModelIndex(), rows.at(i), rows.at(i));
            m_entries.remove(rows.at(i));
            endRemoveRows();
        }
        invalidateDerived();
        return true;
    }

    // Idempotent: the latest definition already carries exactly this body.
    if (!rows.isEmpty()) {
        const cmd::Entry &latest = m_entries.at(rows.last());
        if (latest.args.size() >= 2 && aliasBodyOf(latest) == cleanBody)
            return false;
    }

    const QStringList args = { QStringLiteral("alias"), quoted(cleanName),
                               quoted(cleanBody) };

    // Update the latest definition in place, dropping earlier same-name rows (a later
    // definition wins in TF2's replay, so the others are dead weight).
    if (!rows.isEmpty()) {
        const int dropCount = rows.size() - 1;
        for (int i = rows.size() - 2; i >= 0; --i) {
            beginRemoveRows(QModelIndex(), rows.at(i), rows.at(i));
            m_entries.remove(rows.at(i));
            endRemoveRows();
        }
        const int row = rows.last() - dropCount;
        cmd::Entry &entry = m_entries[row];
        entry.appOwned = true;
        // The existing definition's name spelling stays (the file's spelling wins on
        // an edit; setAlias's name argument only selects the row).
        const QVariant oldName = entry.args.value(1);
        entry.args = { QStringLiteral("alias"), oldName.toString(),
                       quoted(cleanBody) };
        entry.type = cmd::CMD_CMD; // what the parser gives an identical file line
        emit dataChanged(index(row), index(row),
                         {LineRole, CommandRole, ArgsRole, TypeRole, TypeNameRole,
                          BoundKeyRole});
        invalidateDerived();
        return true;
    }

    // New definition: a session row anchored after the last alias line so definitions
    // stay together, else end of file (the bundled presets define aliases after their
    // binds - binds resolve alias names when pressed, not when parsed).
    cmd::Entry entry;
    entry.appOwned = true;
    entry.args = args;
    entry.type = cmd::CMD_CMD;
    entry.anchorLine = cmd::ANCHOR_EOF;
    int lastAliasLine = -1;
    for (const cmd::Entry &e : m_entries) {
        if (isAliasDefinition(e) && e.sourceLine >= 0)
            lastAliasLine = qMax(lastAliasLine, e.sourceLine);
    }
    if (lastAliasLine >= 0)
        entry.anchorLine = lastAliasLine;

    const int row = m_entries.size();
    beginInsertRows(QModelIndex(), row, row);
    m_entries.push_back(entry);
    endInsertRows();
    invalidateDerived();
    return true;
}

QStringList CommandListModel::aliasNames() const {
    QStringList out;
    for (const cmd::Entry &entry : m_entries) {
        if (!isAliasDefinition(entry) || entry.args.size() < 2)
            continue;
        const QString name = unquoted(entry.args.at(1));
        if (!out.contains(name, Qt::CaseInsensitive))
            out << name;
    }
    return out;
}

QVariantMap CommandListModel::aliasInfo(const QString &name) const {
    QVariantMap info;
    info.insert(QStringLiteral("found"), false);
    const QString wanted = unquoted(name).toLower();
    for (int row = m_entries.size() - 1; row >= 0; --row) {
        const cmd::Entry &entry = m_entries.at(row);
        if (!isAliasDefinition(entry) || entry.args.size() < 2)
            continue;
        if (unquoted(entry.args.at(1)).toLower() != wanted)
            continue;
        info.insert(QStringLiteral("found"), true);
        info.insert(QStringLiteral("name"), unquoted(entry.args.at(1)));
        info.insert(QStringLiteral("body"), aliasBodyOf(entry));
        return info;
    }
    return info;
}

QVariantMap CommandListModel::rowInfo(int row) const {
    QVariantMap info;
    if (row < 0 || row >= m_entries.size())
        return info;

    const cmd::Entry &entry = m_entries.at(row);
    info.insert(QStringLiteral("row"), row);
    info.insert(QStringLiteral("line"),
                entry.commentTail.isEmpty()
                    ? entry.args.join(QLatin1Char(' '))
                    : entry.args.join(QLatin1Char(' ')) + QLatin1Char(' ')
                          + entry.commentTail);
    info.insert(QStringLiteral("command"), entry.args.value(0));
    info.insert(QStringLiteral("type"), static_cast<int>(entry.type));
    info.insert(QStringLiteral("typeName"), typeName(entry.type));
    info.insert(QStringLiteral("boundKey"), boundKeyOf(entry));
    ensureScan();
    info.insert(QStringLiteral("warning"), m_warningsCache.value(row, QString()));
    return info;
}

QString CommandListModel::typeName(cmd::CmdType type) {
    switch (type) {
    case cmd::CMD_IVAR:
        return QStringLiteral("int");
    case cmd::CMD_FVAR:
        return QStringLiteral("float");
    case cmd::CMD_SVAR:
        return QStringLiteral("string");
    case cmd::CMD_BVAR:
        return QStringLiteral("bool");
    case cmd::CMD_CMD:
        return QStringLiteral("command");
    case cmd::CMD_BIND:
        return QString(BindSyntax::Bind);
    case cmd::CMD_ALIAS:
        return QStringLiteral("alias");
    case cmd::CMD_COMMENT:
        return QStringLiteral("comment");
    case cmd::CMD_UNKWN:
        break;
    }
    return QStringLiteral("unknown");
}
