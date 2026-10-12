#ifndef CONFIGCONTROLLER_H
#define CONFIGCONTROLLER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QFuture>

#include "gamecfg/gameconfig.h"
#include "ui/commandlistmodel.h"
#include "ui/consolecommandmodel.h"

/**
 * The C++ face of the UI.
 *
 * Owns the parsing engine and the command model, and exposes them to QML as the context
 * property `cfg`. QML never touches GameCfg directly. See docs/ui-qml-rewrite.md
 * section 6 for the full contract and section 10 for where future features plug in.
 */
class ConfigController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *commandModel READ commandModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *consoleModel READ consoleModel CONSTANT)
    Q_PROPERTY(QVariantList presets READ presets CONSTANT)
    Q_PROPERTY(QString cfgPath READ cfgPath NOTIFY cfgPathChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QVariantMap binds READ binds NOTIFY bindsChanged)

public:
    explicit ConfigController(QObject *parent = nullptr);

    QAbstractItemModel *commandModel();
    /// The allcmds.csv catalog (Build-A-Bind source data).
    QAbstractItemModel *consoleModel() { return &m_console; }
    QVariantList presets() const;
    QString cfgPath() const;
    QString lastError() const;
    bool loaded() const;
    bool loading() const { return m_loading; }

    /// Active binds, key token -> command, derived from the parsed command list. The
    /// keyboard paints a cap when its token is a key of this map; a notifying property,
    /// so the caps recolour the moment a bind is added or removed (D16).
    QVariantMap binds() const;

    /// ":/cfg/..." path of a preset, or an empty string for an invalid index.
    Q_INVOKABLE QString presetAt(int index) const;

    /// Short description of a preset, or an empty string.
    Q_INVOKABLE QString presetNote(int index) const;

    /// Parse a config asynchronously and fill the command model. Appends, like the widget build (P37).
    /// Returns immediately; `loading` becomes true, then `loadedChanged`/`lastErrorChanged` fire when done.
    Q_INVOKABLE void loadCfg(const QString &path);

    /// Drop everything that was loaded.
    Q_INVOKABLE void clear();

    /// Bind `command` to TF2 key token `key` (e.g. "space", "kp_home"), replacing any
    /// bind the key already had. An empty command removes the bind instead. In memory
    /// only: the loaded .cfg file is not touched, because the app has no save yet (D16).
    Q_INVOKABLE bool setBind(const QString &key, const QString &command);

    /// Remove the bind of TF2 key token `key`.
    Q_INVOKABLE bool clearBind(const QString &key);

signals:
    void cfgPathChanged();
    void lastErrorChanged();
    void loadedChanged();
    void loadingChanged();
    void bindsChanged();

private:
    void setCfgPath(const QString &path);
    void setLastError(const QString &error);
    void setLoading(bool loading);

    GameCfg m_cfg; ///< default ctor loads res/cfg/allcmds.csv once
    CommandListModel m_commands;
    ConsoleCommandModel m_console;
    QString m_cfgPath;
    QString m_lastError;
    bool m_loading = false;
    QFuture<void> m_loadFuture;
};

#endif // CONFIGCONTROLLER_H
