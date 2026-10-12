#include "consolecommandmodel.h"

ConsoleCommandModel::ConsoleCommandModel(QObject *parent)
    : QAbstractListModel(parent) {}

int ConsoleCommandModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_visible.size());
}

QVariant ConsoleCommandModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_visible.size())
        return {};

    const cmd::ConVar *cmd = m_all.at(m_visible.at(index.row()));

    switch (role) {
    case NameRole:
        return cmd->get_name();
    case DocsRole:
        return cmd->get_docs();
    case InputRole:
        return cmd->get_usage();
    case PlatformRole:
        return cmd->get_platform();
    case ContextRole:
        return cmd->get_context();
    case CheatRole:
        return cmd->isCheat();
    case DefaultRole:
        return cmd->defaultStr();
    case MinRole:
        return cmd->hasRange() ? QString::number(cmd->minVal()) : QString();
    case MaxRole:
        return cmd->hasRange() ? QString::number(cmd->maxVal()) : QString();
    default:
        return {};
    }
}

QHash<int, QByteArray> ConsoleCommandModel::roleNames() const {
    static const QHash<int, QByteArray> names = [] {
        QHash<int, QByteArray> h;
        h[NameRole] = "name";
        h[DocsRole] = "docs";
        h[InputRole] = "input";
        h[PlatformRole] = "platform";
        h[ContextRole] = "context";
        h[CheatRole] = "cheat";
        h[DefaultRole] = "def";
        h[MinRole] = "min";
        h[MaxRole] = "max";
        return h;
    }();
    return names;
}

void ConsoleCommandModel::setCommands(const QVector<cmd::ConVar *> &commands) {
    beginResetModel();
    m_all = commands;
    rebuild();
    endResetModel();
}

void ConsoleCommandModel::setContextFilter(const QString &context) {
    if (m_contextFilter == context)
        return;
    beginResetModel();
    m_contextFilter = context;
    rebuild();
    endResetModel();
}

void ConsoleCommandModel::setPlatformFilter(const QString &platform) {
    if (m_platformFilter == platform)
        return;
    beginResetModel();
    m_platformFilter = platform;
    rebuild();
    endResetModel();
}

void ConsoleCommandModel::rebuild() {
    m_visible.clear();
    for (int i = 0; i < m_all.size(); ++i) {
        const cmd::ConVar *cmd = m_all.at(i);
        if (!m_contextFilter.isEmpty() && cmd->get_context() != m_contextFilter)
            continue;
        if (!m_platformFilter.isEmpty() && cmd->get_platform() != m_platformFilter)
            continue;
        m_visible.append(i);
    }
}

QVariantMap ConsoleCommandModel::commandAt(int row) const {
    if (row < 0 || row >= m_visible.size())
        return {};

    const cmd::ConVar *cmd = m_all.at(m_visible.at(row));
    QVariantMap map;
    map.insert(QStringLiteral("name"), cmd->get_name());
    map.insert(QStringLiteral("docs"), cmd->get_docs());
    map.insert(QStringLiteral("input"), cmd->get_usage());
    map.insert(QStringLiteral("platform"), cmd->get_platform());
    map.insert(QStringLiteral("context"), cmd->get_context());
    map.insert(QStringLiteral("cheat"), cmd->isCheat());
    map.insert(QStringLiteral("def"), cmd->defaultStr());
    map.insert(QStringLiteral("min"), cmd->hasRange() ? QString::number(cmd->minVal()) : QString());
    map.insert(QStringLiteral("max"), cmd->hasRange() ? QString::number(cmd->maxVal()) : QString());
    // A bare type (bool/int/flt/str, no brackets) is a cvar value; commands carry a
    // signature with []/<>/none. The arguments pane switches on this.
    const QString input = cmd->get_usage();
    map.insert(QStringLiteral("isCvar"),
               !input.contains('[') && !input.contains('<') && input != QLatin1String("none"));
    return map;
}
