#ifndef CONFIGCONTROLLER_H
#define CONFIGCONTROLLER_H

#include <QObject>
#include <QString>
#include <QVariantList>

#include "gamecfg/gameconfig.h"
#include "ui/commandlistmodel.h"

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
    Q_PROPERTY(QVariantList presets READ presets CONSTANT)
    Q_PROPERTY(QString cfgPath READ cfgPath NOTIFY cfgPathChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)

public:
    explicit ConfigController(QObject *parent = nullptr);

    QAbstractItemModel *commandModel();
    QVariantList presets() const;
    QString cfgPath() const;
    QString lastError() const;
    bool loaded() const;

    /// ":/cfg/..." path of a preset, or an empty string for an invalid index.
    Q_INVOKABLE QString presetAt(int index) const;

    /// Short description of a preset, or an empty string.
    Q_INVOKABLE QString presetNote(int index) const;

    /// Parse a config and fill the command model. Appends, like the widget build (P37).
    Q_INVOKABLE bool loadCfg(const QString &path);

    /// Drop everything that was loaded.
    Q_INVOKABLE void clear();

signals:
    void cfgPathChanged();
    void lastErrorChanged();
    void loadedChanged();

private:
    void setCfgPath(const QString &path);
    void setLastError(const QString &error);

    GameCfg m_cfg; ///< default ctor loads res/cfg/allcmds.csv once
    CommandListModel m_commands;
    QString m_cfgPath;
    QString m_lastError;
};

#endif // CONFIGCONTROLLER_H
