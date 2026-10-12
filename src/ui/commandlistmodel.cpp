#include "commandlistmodel.h"

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
/// same way. See docs/ui-qml-rewrite.md D16.
QString unquoted(const QString &arg) {
    if (arg.size() >= 2 && arg.startsWith(QLatin1Char('"')) && arg.endsWith(QLatin1Char('"')))
        return arg.mid(1, arg.size() - 2);
    return arg;
}

QString quoted(const QString &arg) {
    return QLatin1Char('"') + arg + QLatin1Char('"');
}

/// True for the two commands that put a command on a key: `bind` and `bindtoggle`
/// (allcmds.csv spells the latter BindToggle; the lexer lowercases every line).
bool isBindCommand(const QString &head) {
    return head == QLatin1String("bind") || head == QLatin1String("bindtoggle");
}

} // namespace

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
    emit bindsChanged();
}

void CommandListModel::clearEntries() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
    m_bindsCacheValid = false;
    emit bindsChanged();
}

QVariantMap CommandListModel::binds() const {
    if (m_bindsCacheValid) {
        return m_bindsCache;
    }

    const BindScanResult result = scanBinds();
    m_bindsCache = result.binds;
    m_bindsCacheValid = true;
    return m_bindsCache;
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

        if (head == QLatin1String("unbindall")) {
            result.binds.clear();
            result.rowsByKey.clear();
            continue;
        }
        if (head == QLatin1String("unbind")) {
            if (entry.args.size() >= 2) {
                const QString key = unquoted(entry.args.at(1)).toLower();
                result.binds.remove(key);
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
        if (command.isEmpty())
            result.binds.remove(key);
        else
            result.binds.insert(key, command);
    }

    return result;
}

bool CommandListModel::setBind(const QString &key, const QString &command) {
    const QString cleanKey = unquoted(key).toLower();
    // The parser lowercases every line it reads, so a row written here has to match or the
    // same edit applied twice would produce two different rows.
    const QString cleanCommand = command.trimmed().toLower();

    if (cleanKey.isEmpty())
        return false;
    if (cleanCommand.isEmpty())
        return clearBind(cleanKey);
    if (binds().value(cleanKey).toString() == cleanCommand)
        return false; // already bound to exactly this

    const QVector<int> rows = bindRows(cleanKey);
    for (int i = rows.size() - 1; i >= 0; --i) {
        const int row = rows.at(i);
        beginRemoveRows(QModelIndex(), row, row);
        m_entries.remove(row);
        endRemoveRows();
    }

    cmd::Entry entry;
    entry.args << QStringLiteral("bind") << quoted(cleanKey) << quoted(cleanCommand);
    // `bind` is a plain console command in allcmds.csv, which is the type the parser gives
    // an identical line in a config file. Keep the two indistinguishable.
    entry.type = cmd::CMD_CMD;

    const int row = m_entries.size();
    beginInsertRows(QModelIndex(), row, row);
    m_entries.push_back(entry);
    endInsertRows();

    m_bindsCacheValid = false;
    emit bindsChanged();
    return true;
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
        return QStringLiteral("bind");
    case cmd::CMD_ALIAS:
        return QStringLiteral("alias");
    case cmd::CMD_UNKWN:
        break;
    }
    return QStringLiteral("unknown");
}
