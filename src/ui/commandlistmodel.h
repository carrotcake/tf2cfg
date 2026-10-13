#ifndef COMMANDLISTMODEL_H
#define COMMANDLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QVariantMap>
#include <QVector>

#include "gamecfg/convars.h"

/**
 * The parsed command list, plus the bind state derived from it.
 *
 * One row per cmd::Entry, in parse order, so the "Command Overview" list shows exactly
 * what the widget build showed. Extra roles (command, arguments, type) exist so the
 * future typed editors, grouping and keyboard overlay do not need this class reopened.
 * See docs/legacy/ui-qml-rewrite.md section 6.
 *
 * The model stops being strictly read-only here: setBind()/clearBind() edit the rows so
 * the keyboard can paint the keys that have a bind (docs/legacy/ui-qml-rewrite.md D16).
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
        BoundKeyRole,                ///< lowercase key token a bind applies to, else ""
    };
    Q_ENUM(Role)

    explicit CommandListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<cmd::Entry> &entries);
    void clearEntries();

    /**
     * Active binds: lowercase, unquoted key token -> command string.
     *
     * Replayed in parse order the way TF2 would, so a later bind wins and `unbind` /
     * `unbindall` clear earlier ones. A bind with an empty command (`bind "r" ""`, which
     * b4nny.cfg uses for e/r/t) leaves the key unbound. Key tokens that have no cap,
     * MOUSE1 and MWHEELUP for instance, are kept: the keyboard simply never asks.
     */
    QVariantMap binds() const;

    /// Bind `command` to key token `key`, replacing every earlier bind of that key.
    /// Adds one `bind "<key>" "<command>"` row at the end of the list, so a key never
    /// ends up with two competing rows. False when nothing changed.
    bool setBind(const QString &key, const QString &command);

    /// Drop every bind row for key token `key`. False when the key had no bind row.
    bool clearBind(const QString &key);

    static QString typeName(cmd::CmdType type);

signals:
    /// Bind state changed: rows were edited, or a whole new command list was parsed.
    void bindsChanged();

private:
    /// Rows that bind or unbind key token `key`, in parse order.
    QVector<int> bindRows(const QString &key) const;

    struct BindScanResult {
        QVariantMap binds;
        QHash<QString, QVector<int>> rowsByKey;
    };
    BindScanResult scanBinds() const;

    QVector<cmd::Entry> m_entries;
    mutable QVariantMap m_bindsCache;
    mutable bool m_bindsCacheValid = false;
};

#endif // COMMANDLISTMODEL_H
