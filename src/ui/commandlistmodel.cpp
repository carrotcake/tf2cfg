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
    return names;
}

/// The config lexer hands arguments back with their quotes still on, so `bind "0" "slot10"`
/// parses as three arguments: `bind`, `"0"` and `"slot10"`. Both directions are needed:
/// read a key/command out of a parsed row, and write a row the parser would reparse the
/// same way. See docs/legacy/ui-qml-rewrite.md D16.
///
/// The lexer keeps the backslashes a quoted token carries, so this is the one place that
/// reads them - which is why a row setBind() wrote and the same row read back from a file
/// both come out as one identical command.
QString unquoted(const QString &arg) {
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

QString quoted(const QString &arg) {
    QString out;
    for (const QChar c : arg) {
        if (c == QLatin1Char('"') || c == QLatin1Char('\\'))
            out.append(QLatin1Char('\\'));
        out.append(c);
    }
    return QLatin1Char('"') + out + QLatin1Char('"');
}

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
        return entry.args.join(" "); // parity P38: same text as the widget list
    case CommandRole:
        return entry.args.value(0);
    case ArgsRole:
        return entry.args.mid(1);
    case TypeRole:
        return static_cast<int>(entry.type);
    case TypeNameRole:
        return typeName(entry.type);
    case BoundKeyRole: {
        if (entry.args.isEmpty())
            return QString();
        const QString head = unquoted(entry.args.at(0)).toLower();
        if (!isBindCommand(head) || entry.args.size() < 2)
            return QString();
        return unquoted(entry.args.at(1)).toLower();
    }
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
    endResetModel();
    m_bindsCacheValid = false;
    m_bindingKindsCacheValid = false;
    emit bindsChanged();
}

void CommandListModel::clearEntries() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
    m_bindsCacheValid = false;
    m_bindingKindsCacheValid = false;
    emit bindsChanged();
}

QVariantMap CommandListModel::binds() const {
    if (m_bindsCacheValid) {
        return m_bindsCache;
    }

    const BindScanResult result = scanBinds();
    m_bindsCache = result.binds;
    m_bindingKindsCache = result.bindingKinds;
    m_bindsCacheValid = true;
    m_bindingKindsCacheValid = true;
    return m_bindsCache;
}

QVariantMap CommandListModel::bindingKinds() const {
    if (m_bindingKindsCacheValid) {
        return m_bindingKindsCache;
    }

    const BindScanResult result = scanBinds();
    m_bindsCache = result.binds;
    m_bindingKindsCache = result.bindingKinds;
    m_bindsCacheValid = true;
    m_bindingKindsCacheValid = true;
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

CommandListModel::BindScanResult CommandListModel::scanBinds() const {
    BindScanResult result;

    for (int row = 0; row < m_entries.size(); ++row) {
        const cmd::Entry &entry = m_entries.at(row);
        if (entry.args.isEmpty())
            continue;

        const QString head = unquoted(entry.args.at(0)).toLower();

        if (head == BindSyntax::UnbindAll) {
            result.binds.clear();
            result.bindingKinds.clear();
            result.rowsByKey.clear();
            continue;
        }
        if (head == BindSyntax::Unbind) {
            if (entry.args.size() >= 2) {
                const QString key = unquoted(entry.args.at(1)).toLower();
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
        }
    }

    return result;
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
        m_bindsCacheValid = false;
        m_bindingKindsCacheValid = false;
        emit bindsChanged();
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

    m_bindsCacheValid = false;
    m_bindingKindsCacheValid = false;
    emit bindsChanged();
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

    m_bindsCacheValid = false;
    m_bindingKindsCacheValid = false;
    emit bindsChanged();
    return true;
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
    case cmd::CMD_UNKWN:
        break;
    }
    return QStringLiteral("unknown");
}
