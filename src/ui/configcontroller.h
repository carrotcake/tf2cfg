#ifndef CONFIGCONTROLLER_H
#define CONFIGCONTROLLER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QDateTime>
#include <QFuture>

#include "gamecfg/gameconfig.h"
#include "ui/commandfiltermodel.h"
#include "ui/commandlistmodel.h"
#include "ui/consolecommandmodel.h"

/**
 * The C++ face of the UI.
 *
 * Owns the parsing engine and the command model, and exposes them to QML as the context
 * property `cfg`. QML never touches GameCfg directly. See docs/legacy/ui-qml-rewrite.md
 * section 6 for the full contract and section 10 for where future features plug in.
 */
class ConfigController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *commandModel READ commandModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *consoleModel READ consoleModel CONSTANT)
    /// The Config Editor's filtered view of commandModel (docs/DESIGN-config-editor.md §2).
    Q_PROPERTY(QAbstractItemModel *rowFilter READ rowFilter CONSTANT)
    Q_PROPERTY(QVariantList presets READ presets CONSTANT)
    Q_PROPERTY(QString cfgPath READ cfgPath NOTIFY cfgPathChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    /// True when any app-owned row exists, i.e. Save would change the file. Notified
    /// through bindsChanged, which fires on every model change that can alter it.
    Q_PROPERTY(bool dirty READ isDirty NOTIFY bindsChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QVariantMap binds READ binds NOTIFY bindsChanged)
    Q_PROPERTY(QVariantMap bindingKinds READ bindingKinds NOTIFY bindsChanged)

public:
    explicit ConfigController(QObject *parent = nullptr);

    QAbstractItemModel *commandModel();
    /// The allcmds.csv catalog (Build-A-Bind source data).
    QAbstractItemModel *consoleModel() { return &m_console; }
    QAbstractItemModel *rowFilter() { return &m_rowFilter; }

    /// One row as a QVariantMap for the Config Editor's pane (delegates to the model).
    Q_INVOKABLE QVariantMap rowInfo(int row) const { return m_commands.rowInfo(row); }
    QVariantList presets() const;
    QString cfgPath() const;
    QString lastError() const;
    bool loaded() const;
    bool loading() const { return m_loading; }

    /// Active binds, key token -> command, derived from the parsed command list. The
    /// keyboard paints a cap when its token is a key of this map; a notifying property,
    /// so the caps recolour the moment a bind is added or removed (D16).
    QVariantMap binds() const;

    /// Active binding kinds, key token -> "bind" or "bindtoggle", updated alongside
    /// binds (audit #6), so a future key cap can show toggle-specific text.
    QVariantMap bindingKinds() const;

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
    /// bind the key already had. An empty command removes the bind instead. Edits the
    /// model; it reaches the file on the next saveCfg() (D16, D20).
    Q_INVOKABLE bool setBind(const QString &key, const QString &command);
    Q_INVOKABLE bool setBindToggle(const QString &key, const QString &command);

    /// Remove the bind of TF2 key token `key`.
    Q_INVOKABLE bool clearBind(const QString &key);

    /// "bind" or "bindtoggle" for key token `key`, else "".
    Q_INVOKABLE QString bindingKind(const QString &key) const;

    // -- Config Editor (docs/DESIGN-config-editor.md) --------------------------------

    /// Tokenize one command line the way the parser would and classify its head.
    /// { ok, args, typeName, error }: ok is false for an empty/comment-only line.
    /// The guided editors (§8) reverse-parse custom text through this, so there is
    /// one tokenizer, not a second one in QML.
    Q_INVOKABLE QVariantMap parseCommandLine(const QString &line) const;

    /// Replace row `row` with the tokenized text (raw edit). False when the text
    /// parses to nothing.
    Q_INVOKABLE bool setRowText(int row, const QString &text);

    /// Remove row `row`; the save drops its line.
    Q_INVOKABLE bool deleteRow(int row);

    /// Insert a new row after `anchorRow` (its file line is the save anchor;
    /// anchorRow < 0 appends at end of file). Returns the new row index, -1 on
    /// refusal.
    Q_INVOKABLE int insertRowAfter(int anchorRow, const QString &text);

    // -- aliases + chains (docs/DESIGN-config-editor.md §5) --------------------------

    /// Define/update/remove an alias (empty body removes). Writes the `alias` row
    /// the next save regenerates.
    Q_INVOKABLE bool setAlias(const QString &name, const QString &body);

    /// Names defined by `alias` rows in the loaded config.
    Q_INVOKABLE QStringList aliasNames() const;

    /// { found, name, body } of the latest `alias <name>` definition.
    Q_INVOKABLE QVariantMap aliasInfo(const QString &name) const;

    /// Split a command line on unquoted `;` (quote- and escape-aware, the lexer's
    /// rules), trimmed; the chain editor's segments (§5). Single-segment lines come
    /// back as a one-element list.
    Q_INVOKABLE QStringList splitSegments(const QString &line) const;

    /// Token round-trips (D19): quoteToken escapes and wraps raw text; unquoteToken
    /// reads a parsed token back to raw text.
    Q_INVOKABLE QString quoteToken(const QString &raw) const;
    Q_INVOKABLE QString unquoteToken(const QString &token) const;

    /**
     * Surgical save (docs/legacy/ui-qml-rewrite.md 10.1): rewrite only the lines the
     * app owns, copy every other line byte-for-byte. Writes `<name>.bak` beside the
     * file first, then the file itself through QSaveFile.
     *
     * QML drives this as: ask canSave() (false = refuse, explain via saveBlockReason),
     * then needsConfirm() (true = the file changed on disk; show a dialog), then call
     * saveCfg() or saveCfgAs(path). Keeping the questions separate from the write lets
     * the QML dialog sit between them.
     */
    Q_INVOKABLE bool canSave() const;
    Q_INVOKABLE QString saveBlockReason() const;
    Q_INVOKABLE bool needsConfirm() const;
    Q_INVOKABLE bool saveCfg();
    Q_INVOKABLE bool saveCfgAs(const QString &path);

    /// True when any app-owned row exists, i.e. saving would change the file. Drives
    /// the Save action's enabled state.
    Q_INVOKABLE bool isDirty() const;

signals:
    void cfgPathChanged();
    void lastErrorChanged();
    void loadedChanged();
    void loadingChanged();
    void bindsChanged();
    /// After a successful save, so QML can show transient feedback.
    void saved(const QString &path);

private:
    void setCfgPath(const QString &path);
    void setLastError(const QString &error);
    void setLoading(bool loading);
    /// `source` is the surgical baseline: the file whose bytes every non-owned line is
    /// copied from. Plain save rewrites `path` itself, so source == path; Save As seeds
    /// the new file from the loaded config instead, which may be a `:/` resource.
    bool writeConfig(const QString &path, const QString &source, QString *error);
    /// The `.bak`-then-`QSaveFile` tail both write paths share.
    bool commitWrite(const QString &path, const QByteArray &out, QString *error);
    /// Rebuild rows + parse snapshot from the file just written; see the .cpp.
    void resyncAfterSave();
    void recordFileStamp();

    GameCfg m_cfg; ///< default ctor loads res/cfg/allcmds.csv once
    CommandListModel m_commands;
    CommandFilterModel m_rowFilter; ///< over m_commands; set in the ctor
    ConsoleCommandModel m_console;
    /// Rows exactly as the parse produced them. The save compares this against the live
    /// model: a line the parse covered but whose row the user since deleted is dropped
    /// from the file instead of surviving verbatim.
    QVector<cmd::Entry> m_parsed;
    QString m_cfgPath;
    QString m_lastError;
    bool m_loading = false;
    /// Parsed rows deleted since load/save; each will drop its file line on save, so
    /// dirty must be true even with no app-owned row left (docs/DESIGN-config-editor.md).
    int m_droppedParsedLines = 0;
    QFuture<void> m_loadFuture;
    QDateTime m_loadedStamp;  ///< file mtime at load; compared at save
    qint64 m_loadedSize = -1; ///< file size at load
};

#endif // CONFIGCONTROLLER_H
