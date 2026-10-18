#include "commandfiltermodel.h"

#include "ui/commandlistmodel.h"

#include "gamecfg/convars.h"

CommandFilterModel::CommandFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent) {}

void CommandFilterModel::setFilterText(const QString &text) {
    if (m_filterText == text)
        return;
    m_filterText = text;
    emit filterTextChanged();
    invalidateFilter();
}

void CommandFilterModel::setFilterKind(int kind) {
    if (m_filterKind == kind)
        return;
    m_filterKind = kind;
    emit filterKindChanged();
    invalidateFilter();
}

bool CommandFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);

    if (m_filterKind >= 0) {
        const int type = idx.data(CommandListModel::TypeRole).toInt();
        const bool bound = !idx.data(CommandListModel::BoundKeyRole).toString().isEmpty();
        bool match = false;
        switch (m_filterKind) {
        case 0: // cvars
            match = type == cmd::CMD_IVAR || type == cmd::CMD_FVAR || type == cmd::CMD_SVAR
                    || type == cmd::CMD_BVAR;
            break;
        case 1: // commands (binds excluded, they have their own kind)
            match = type == cmd::CMD_CMD && !bound;
            break;
        case 2: // aliases: `alias` definition rows parse as CMD_CMD (alias is a catalog
            // command); rows referencing a file-defined alias parse as CMD_ALIAS.
            match = type == cmd::CMD_ALIAS
                    || idx.data(CommandListModel::CommandRole).toString()
                           .compare(QLatin1String("alias"), Qt::CaseInsensitive) == 0;
            break;
        case 3: // not in catalog
            match = type == cmd::CMD_UNKWN;
            break;
        case 4: // binds (redirect to Edit Binds)
            match = bound;
            break;
        case 5: // comments (§6)
            match = type == cmd::CMD_COMMENT;
            break;
        default:
            break;
        }
        if (!match)
            return false;
    }

    if (m_filterText.isEmpty())
        return true;
    return idx.data(CommandListModel::LineRole).toString().contains(m_filterText,
                                                                    Qt::CaseInsensitive);
}