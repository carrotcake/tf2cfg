#include "consolecommandmodel.h"

#include <algorithm>

uint qHash(const ConsoleCommandModel::FilterKey &key, uint seed = 0) noexcept {
    uint h = seed;
    h = qHash(key.context, h);
    h = qHash(key.platform, h);
    h = qHash(key.showCheats, h);
    return h;
}

/// Bounds and defaults for the argument editors. QString::number's default 'g' format
/// renders 10000000.0 as "1e+07" (sensitivity's maximum), so the precision is raised far
/// enough to print whole numbers in full; 15 significant digits still round-trips a double.
QString number(double value) {
    return QString::number(value, 'g', 15);
}

// Build-A-Bind sort: names order by their alphanumeric characters only, so
// "+attack" sorts as "attack". Case-insensitive.
static QString commandSortKey(const QString &name) {
    QString key;
    for (const QChar c : name) {
        if (c.isLetterOrNumber())
            key.append(c.toLower());
    }
    return key;
}

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
        return cmd->hasRange() ? number(cmd->minVal()) : QString();
    case MaxRole:
        return cmd->hasRange() ? number(cmd->maxVal()) : QString();
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

void ConsoleCommandModel::setShowCheats(bool show) {
    if (m_showCheats == show)
        return;
    beginResetModel();
    m_showCheats = show;
    rebuild();
    endResetModel();
}

void ConsoleCommandModel::rebuild() {
    m_currentFilterKey = {m_contextFilter, m_platformFilter, m_showCheats};

    auto it = m_filterCache.find(m_currentFilterKey);
    if (it == m_filterCache.end()) {
        // First time seeing this filter combo: build and cache.
        FilterCache cache;
        QVector<QPair<QString, int>> keyed;
        keyed.reserve(m_all.size());
        for (int i = 0; i < m_all.size(); ++i) {
            const cmd::ConVar *cmd = m_all.at(i);
            if (!m_contextFilter.isEmpty() && cmd->get_context() != m_contextFilter)
                continue;
            if (!m_platformFilter.isEmpty() && cmd->get_platform() != m_platformFilter)
                continue;
            if (!m_showCheats && cmd->isCheat())
                continue;
            keyed.append({commandSortKey(cmd->get_name()), i});
        }
        std::sort(keyed.begin(), keyed.end(),
                  [this](const QPair<QString, int> &a, const QPair<QString, int> &b) {
            if (a.first != b.first)
                return a.first < b.first;
            return QString::compare(m_all.at(a.second)->get_name(),
                                    m_all.at(b.second)->get_name(), Qt::CaseInsensitive) < 0;
        });
        cache.visibleIndices.resize(keyed.size());
        for (int row = 0; row < keyed.size(); ++row)
            cache.visibleIndices[row] = keyed.at(row).second;
        m_filterCache.insert(m_currentFilterKey, std::move(cache));
        it = m_filterCache.find(m_currentFilterKey);
    }
    m_visible = it->visibleIndices;
}

QVariantMap ConsoleCommandModel::findCommand(const QString &name) const {
    for (const cmd::ConVar *cmd : m_all) {
        if (QString::compare(cmd->get_name(), name, Qt::CaseInsensitive) == 0) {
            QVariantMap map;
            map.insert(QStringLiteral("name"), cmd->get_name());
            map.insert(QStringLiteral("context"), cmd->get_context());
            map.insert(QStringLiteral("platform"), cmd->get_platform());
            map.insert(QStringLiteral("cheat"), cmd->isCheat());
            return map;
        }
    }
    return {};
}

QStringList ConsoleCommandModel::suggest(const QString &text, int limit) const {
    QStringList out;
    if (text.isEmpty())
        return out;

    // Use the cached filtered indices for the current filter combo.
    auto it = m_filterCache.find(m_currentFilterKey);
    if (it == m_filterCache.end())
        return out; // no visible entries for this filter combo

    const QVector<int> &candidates = it->visibleIndices;
    const QString needle = text.toLower();

    for (int idx : candidates) {
        if (m_all.at(idx)->get_name().contains(needle, Qt::CaseInsensitive)) {
            out.append(m_all.at(idx)->get_name());
            if (out.size() >= limit)
                break;
        }
    }
    return out;
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
    map.insert(QStringLiteral("min"), cmd->hasRange() ? number(cmd->minVal()) : QString());
    map.insert(QStringLiteral("max"), cmd->hasRange() ? number(cmd->maxVal()) : QString());
    // A bare type (bool/int/flt/str, no brackets) is a cvar value; commands carry a
    // signature with []/<>/none. The arguments pane switches on this.
    const QString input = cmd->get_usage();
    map.insert(QStringLiteral("isCvar"),
               !input.contains('[') && !input.contains('<') && input != QLatin1String("none"));
    return map;
}
