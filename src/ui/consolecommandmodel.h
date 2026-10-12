#ifndef CONSOLECOMMANDMODEL_H
#define CONSOLECOMMANDMODEL_H

#include <QAbstractListModel>
#include <QVariantMap>
#include <QVector>

#include "gamecfg/convars.h"

/**
 * The allcmds.csv catalog for the Build-A-Bind UI: one row per console entry with
 * its docs, input-type signature, platform and context. Rows are filtered by the
 * contextFilter / platformFilter properties (empty string = no filter); QML binds
 * the Category and Platform dropdowns straight onto those.
 *
 * This is the *game's* command catalog. CommandListModel is the *parsed config*
 * list - the two are intentionally separate.
 */
class ConsoleCommandModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString contextFilter READ contextFilter WRITE setContextFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString platformFilter READ platformFilter WRITE setPlatformFilter NOTIFY filtersChanged)
    /// false hides commands flagged "cheat" from the list and suggestions.
    Q_PROPERTY(bool showCheats READ showCheats WRITE setShowCheats NOTIFY filtersChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        DocsRole,
        InputRole,    ///< column 8: cvar type or [mandatory]/<optional> signature
        PlatformRole, ///< column 9: universal/linux/windows/xbox
        ContextRole,  ///< column 10: client controls/settings/render, server ...
        CheatRole,    ///< flags contain "cheat"
        DefaultRole,  ///< cvar default as string, "" when none
        MinRole,      ///< range lower bound as string, "" when unbounded
        MaxRole,      ///< range upper bound as string, "" when unbounded
    };
    Q_ENUM(Role)

    explicit ConsoleCommandModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString contextFilter() const { return m_contextFilter; }
    QString platformFilter() const { return m_platformFilter; }
    bool showCheats() const { return m_showCheats; }
    void setContextFilter(const QString &context);
    void setPlatformFilter(const QString &platform);
    void setShowCheats(bool show);

    /// Full record of one visible row, for the Build-A-Bind arguments pane:
    /// name, docs, input, platform, context, cheat, def, min, max.
    Q_INVOKABLE QVariantMap commandAt(int row) const;

    /// Record (name, context, platform) for the named command, found
    /// case-insensitively anywhere in the catalog. Empty map when unknown.
    Q_INVOKABLE QVariantMap findCommand(const QString &name) const;

    /// Names (in the model's sorted order) containing the text,
    /// case-insensitive, restricted to the active filters, capped at limit.
    Q_INVOKABLE QStringList suggest(const QString &text, int limit = 20) const;

    void setCommands(const QVector<cmd::ConVar *> &commands);

signals:
    void filtersChanged();

private:
    void rebuild();

    QVector<cmd::ConVar *> m_all;   ///< borrowed from GameCfg, which outlives the model
    QVector<int> m_visible;         ///< indices into m_all passing both filters
    QString m_contextFilter;
    QString m_platformFilter;
    bool m_showCheats = true;
};

#endif // CONSOLECOMMANDMODEL_H
