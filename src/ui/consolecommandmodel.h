#ifndef CONSOLECOMMANDMODEL_H
#define CONSOLECOMMANDMODEL_H

#include <QAbstractListModel>
#include <QVariantMap>
#include <QVector>
#include <QtQmlIntegration/qqmlintegration.h>

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
    /// ConfigController owns it; QML reaches it through Cfg.consoleModel.
    QML_ANONYMOUS

    Q_PROPERTY(QString contextFilter READ contextFilter WRITE setContextFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString platformFilter READ platformFilter WRITE setPlatformFilter NOTIFY filtersChanged)
    /// false hides commands flagged "cheat" from the list and suggestions.
    Q_PROPERTY(bool showCheats READ showCheats WRITE setShowCheats NOTIFY filtersChanged)
    /// The platform filter menu, in order: one {value, label} per offered platform. The
    /// vocabulary itself is kPlatformOptions below - the Build-A-Bind dialog's Platform
    /// dropdown and the Command Overview's platform column both read it, so they cannot
    /// disagree about what a platform is called.
    Q_PROPERTY(QList<QVariant> platformOptions READ platformOptions CONSTANT)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        DocsRole,
        InputRole,    ///< column 8: cvar type or [mandatory]/<optional> signature
        PlatformRole, ///< column 9: universal/linux/windows/xbox
        ContextRole,  ///< column 10: client controls/settings/render, server ...
        CheatRole,    ///< flags contain "cheat"
        DefaultRole,  ///< cvar default as string, "" when none
        MinRole,      ///< cvar range lower bound as string, "" when unbounded
        MaxRole,      ///< cvar range upper bound as string, "" when unbounded
        ArgsSpecRole, ///< per-argument specs of a command: list of {index, isFloat,
                      ///< hasMin, min, hasMax, max, optional}; empty for cvars
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

    /// Full record (same fields as commandAt) for the named command, found
    /// case-insensitively anywhere in the catalog. Empty map when unknown.
    Q_INVOKABLE QVariantMap findCommand(const QString &name) const;

    /// The words the UI shows for a catalog platform value ("linux" -> "Linux-only"),
    /// used for the rows that carry one. A value the vocabulary does not know is
    /// returned unchanged, so it names itself instead of rendering blank.
    Q_INVOKABLE QString platformLabel(const QString &platform) const;

    /// The platform filter menu (kPlatformOptions, offered entries only).
    QList<QVariant> platformOptions() const;

    /// Names (in the model's sorted order) containing the text,
    /// case-insensitive, restricted to the active filters, capped at limit.
    Q_INVOKABLE QStringList suggest(const QString &text, int limit = 20) const;

    void setCommands(const QVector<cmd::ConVar *> &commands);

signals:
    void filtersChanged();

private:
    void rebuild();
    /// The record both commandAt() and findCommand() hand the argument panes.
    QVariantMap fullRecord(const cmd::ConVar *cmd) const;

    // Filter cache key: context + platform + cheat visibility.
    struct FilterKey {
        QString context;
        QString platform;
        bool showCheats;
        bool operator==(const FilterKey &other) const noexcept {
            return context == other.context && platform == other.platform && showCheats == other.showCheats;
        }
    };

    struct FilterCache {
        QVector<int> visibleIndices; ///< sorted indices into m_all passing filters
    };

    // Declare qHash as friend so it can access private FilterKey members.
    friend uint qHash(const FilterKey &key, uint seed) noexcept;

    QVector<cmd::ConVar *> m_all;              ///< borrowed from GameCfg, which outlives the model
    QHash<QString, int> m_commandIndex;         ///< lowercased name -> index in m_all (built once; catalog never reloads)
    QVector<int> m_visible;                    ///< indices into m_all passing both filters
    QString m_contextFilter;
    QString m_platformFilter;
    bool m_showCheats = true;
    FilterKey m_currentFilterKey;              ///< key for the active filter combo
    QHash<FilterKey, FilterCache> m_filterCache; ///< per-combo cached visible indices
};

#endif // CONSOLECOMMANDMODEL_H
