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
    case BoundKeyRole:
        return QString(); // reserved: no bind data yet
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
}

void CommandListModel::clearEntries() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

QString CommandListModel::typeName(cmd::CmdType type) {
    switch (type) {
    case cmd::CMD_IVAR:
        return QStringLiteral("int");
    case cmd::CMD_FVAR:
        return QStringLiteral("float");
    case cmd::CMD_SVAR:
        return QStringLiteral("string");
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
