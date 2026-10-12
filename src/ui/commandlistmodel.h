#ifndef COMMANDLISTMODEL_H
#define COMMANDLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

#include "gamecfg/convars.h"

/**
 * Read-only model over the parsed command list.
 *
 * One row per cmd::Entry, in parse order, so the "Command Overview" list shows exactly
 * what the widget build showed. Extra roles (command, arguments, type) exist so the
 * future typed editors, grouping and keyboard overlay do not need this class reopened.
 * See docs/ui-qml-rewrite.md section 6.
 */
class CommandListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        LineRole = Qt::UserRole + 1, ///< args.join(" ") - the list text
        CommandRole,                 ///< args.value(0)
        ArgsRole,                    ///< args.mid(1)
        TypeRole,                    ///< cmd::CmdType as int
        TypeNameRole,                ///< human-readable type
        BoundKeyRole,                ///< reserved: key token a bind applies to
    };
    Q_ENUM(Role)

    explicit CommandListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<cmd::Entry> &entries);
    void clearEntries();

    static QString typeName(cmd::CmdType type);

private:
    QVector<cmd::Entry> m_entries;
};

#endif // COMMANDLISTMODEL_H
