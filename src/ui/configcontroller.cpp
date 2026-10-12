#include "configcontroller.h"

#include <QFile>
#include <QVariantMap>

ConfigController::ConfigController(QObject *parent)
    : QObject(parent) {
    // GameCfg's default constructor already ran loadAllCmds(); calling it again would
    // duplicate every known command, so it is deliberately not repeated here.

    // The bind map belongs to the model (it owns the rows); QML sees it as cfg.binds.
    connect(&m_commands, &CommandListModel::bindsChanged, this, &ConfigController::bindsChanged);

    // Build-A-Bind browses the game's command catalog, not the parsed config.
    m_console.setCommands(m_cfg.allCmds());
}

QAbstractItemModel *ConfigController::commandModel() {
    return &m_commands;
}

QVariantList ConfigController::presets() const {
    // Parity P9/P10: the widget build's two presets, same labels, same resource paths.
    // Future presets (b4nny_defbinds, b4nny_nobinds) are added here - the combo box and
    // the whole start screen are data-driven.
    return {
        QVariantMap {
            { QStringLiteral("label"), QStringLiteral("b4nny (config.cfg from b4nny.zip)") },
            { QStringLiteral("path"), QStringLiteral(":/cfg/b4nny.cfg") },
            { QStringLiteral("note"), QStringLiteral("Pro-player config: crosshair, viewmodel and net settings") },
        },
        QVariantMap {
            { QStringLiteral("label"), QStringLiteral("Stock TF2 config.cfg") },
            { QStringLiteral("path"), QStringLiteral(":/cfg/default.cfg") },
            { QStringLiteral("note"), QStringLiteral("Valve's default config and binds") },
        },
    };
}

QString ConfigController::presetAt(int index) const {
    const QVariantList list = presets();
    if (index < 0 || index >= list.size())
        return QString();
    return list.at(index).toMap().value(QStringLiteral("path")).toString();
}

QString ConfigController::presetNote(int index) const {
    const QVariantList list = presets();
    if (index < 0 || index >= list.size())
        return QString();
    return list.at(index).toMap().value(QStringLiteral("note")).toString();
}

QString ConfigController::cfgPath() const {
    return m_cfgPath;
}

QString ConfigController::lastError() const {
    return m_lastError;
}

bool ConfigController::loaded() const {
    return m_commands.rowCount() > 0;
}

QVariantMap ConfigController::binds() const {
    return m_commands.binds();
}

bool ConfigController::setBind(const QString &key, const QString &command) {
    return m_commands.setBind(key, command);
}

bool ConfigController::clearBind(const QString &key) {
    return m_commands.clearBind(key);
}

bool ConfigController::loadCfg(const QString &path) {
    if (path.isEmpty()) {
        setLastError(QStringLiteral("no config selected"));
        return false;
    }

    // Review finding 6: the engine ignores Lexers::Lexer::open()'s result and would
    // silently produce an empty list. Same check, surfaced instead of swallowed.
    if (!QFile::exists(path)) {
        setLastError(QStringLiteral("file not found"));
        return false;
    }

    m_cfg.appendCfg(path);
    m_cfg.printCfg(); // parity P12/D7: the widget build dumped the parsed list to stdout

    m_commands.setEntries(m_cfg.getCmdList());
    setCfgPath(path);
    setLastError(QString());
    emit loadedChanged();
    return true;
}

void ConfigController::clear() {
    m_commands.clearEntries();
    setCfgPath(QString());
    setLastError(QString());
    emit loadedChanged();
}

void ConfigController::setCfgPath(const QString &path) {
    if (m_cfgPath == path)
        return;
    m_cfgPath = path;
    emit cfgPathChanged();
}

void ConfigController::setLastError(const QString &error) {
    if (m_lastError == error)
        return;
    m_lastError = error;
    emit lastErrorChanged();
}
