#ifndef COMMANDFILTERMODEL_H
#define COMMANDFILTERMODEL_H

#include <QSortFilterProxyModel>

/**
 * Search + type filter for the Config Editor's row list (docs/DESIGN-config-editor.md
 * §2). Sits over CommandListModel; RowRole flows through so the delegate edits by the
 * source row and the proxy never renumbers anything the editor holds onto.
 */
class CommandFilterModel : public QSortFilterProxyModel {
    Q_OBJECT

    /// Plain substring match on the row text, case-insensitive; empty matches all.
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    /// -1 = every kind; else a Config Editor filter kind:
    /// 0 cvars, 1 commands, 2 aliases, 3 not in catalog, 4 binds.
    Q_PROPERTY(int filterKind READ filterKind WRITE setFilterKind NOTIFY filterKindChanged)

public:
    explicit CommandFilterModel(QObject *parent = nullptr);

    QString filterText() const { return m_filterText; }
    void setFilterText(const QString &text);

    int filterKind() const { return m_filterKind; }
    void setFilterKind(int kind);

signals:
    void filterTextChanged();
    void filterKindChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_filterText;
    int m_filterKind = -1;
};

#endif