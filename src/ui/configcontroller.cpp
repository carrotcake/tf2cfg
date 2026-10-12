#include "configcontroller.h"

#include <QFile>
#include <QVariantMap>
#include <QtConcurrent>

ConfigController::ConfigController(QObject *parent)
    : QObject(parent) {
    // GameCfg's default constructor already ran loadAllCmds(); calling it again would
    // duplicate every known command, so it is deliberately not repeated here.

    // The bind map belongs to the model (it owns the rows); QML sees it as cfg.binds.
    connect(&m_commands, &CommandListModel::bindsChanged, this, &ConfigController::bindsChanged);

    // Build-A-Bind browses the game's command catalog, not the parsed config.
    m_console.setCommands(m_cfg.allCmds());
}

void ConfigController::setLoading(bool loading) {
    if (m_loading == loading)
        return;
    m_loading = loading;
    emit loadingChanged();
}

QAbstractItemModel *ConfigController::commandModel() {
    return &m_commands;
}

QVariantList ConfigController::presets() const {
    return {
        QVariantMap {
            { QStringLiteral("label"), QStringLiteral("b4nny (config.cfg from b4nny.zip)") },
            { QStringLiteral("path"), QStringLiteral(":/cfg/b4nny.cfg") },
            { QStringLiteral("note"), QStringLiteral("Pro-player config: crosshair, viewmodel and net settings") },
        },
        QVariantMap {
            { QStringLiteral("label"), QStringLiteral("b4nny (default binds only)") },
            { QStringLiteral("path"), QStringLiteral(":/cfg/b4nny_defbinds.cfg") },
            { QStringLiteral("note"), QStringLiteral("b4nny's settings + stock default binds") },
        },
        QVariantMap {
            { QStringLiteral("label"), QStringLiteral("b4nny (no binds)") },
            { QStringLiteral("path"), QStringLiteral(":/cfg/b4nny_nobinds.cfg") },
            { QStringLiteral("note"), QStringLiteral("b4nny's settings only, zero binds") },
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

void ConfigController::loadCfg(const QString &path) {
    if (path.isEmpty()) {
        setLastError(QStringLiteral("no config selected"));
        emit loadedChanged();
        return;
    }

    // Review finding 6: the engine ignores Lexers::Lexer::open()'s result and would
    // silently produce an empty list. Same check, surfaced instead of swallowed.
    if (!QFile::exists(path)) {
        setLastError(QStringLiteral("file not found"));
        emit loadedChanged();
        return;
    }

    setLoading(true);
    setLastError(QString());

    // Run parsing on a worker thread so the GUI stays responsive.
    m_loadFuture = QtConcurrent::run([this, path]() {
        m_cfg.appendCfg(path);
        m_cfg.printCfg(); // parity P12/D7: the widget build dumped the parsed list to stdout

        // Must update the model on the GUI thread.
        QMetaObject::invokeMethod(this, [this, path]() {
            m_commands.setEntries(m_cfg.getCmdList());
            setCfgPath(path);
            setLastError(QString());
            setLoading(false);
            emit loadedChanged();
        }, Qt::QueuedConnection);
    });
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
