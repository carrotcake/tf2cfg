#ifndef COMMANDLISTMODEL_H
#define COMMANDLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QVariantMap>
#include <QVector>
#include <QtQmlIntegration/qqmlintegration.h>

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
    /// ConfigController owns it and QML only reaches it through Cfg.commandModel, so QML
    /// needs the metadata (member checking), not a way to construct one.
    QML_ANONYMOUS

public:
    enum Role {
        LineRole = Qt::UserRole + 1, ///< args.join(" ") - the list text
        CommandRole,                 ///< args.value(0)
        ArgsRole,                    ///< args.mid(1)
        TypeRole,                    ///< cmd::CmdType as int
        TypeNameRole,                ///< human-readable type
        BoundKeyRole,                ///< lowercase key token a bind applies to, else ""
        WarningRole,                 ///< replay-order warning text (§7), else ""
        RowRole,                     ///< source-model row; a filter proxy hides rows but
                                     ///< never renumbers (the Config Editor edits by row)
    };
    Q_ENUM(Role)

    explicit CommandListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<cmd::Entry> &entries);
    void clearEntries();

    /// The rows as they currently stand, edits included - the surgical save reads this,
    /// not the parse output, because setBind/clearBind edit the model's copy.
    const QVector<cmd::Entry> &entries() const { return m_entries; }

    /// True for the two commands that put a command on a key (`bind`, `bindtoggle`).
    /// The save uses it to find the line new binds are inserted after.
    static bool isBindCommand(const QString &head);

    /// The lexer hands tokens back with their quotes still on, so `bind "0" "slot10"`
    /// parses as three arguments. unquoted() reads the escapes (`\"`, `\\`) a quoted
    /// token carries; quoted() writes a token the parser reads back identically (D19).
    /// Public: the Config Editor's chain editor and the alias API re-quote bodies.
    static QString unquoted(const QString &arg);
    static QString quoted(const QString &arg);

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
    bool setBind(const QString &key, const QString &command, bool toggle = false);

    /// Bind a key as a TF2 `bindtoggle` row (`bindtoggle "<key>" "<command>"`)
    /// instead of a plain `bind`. Used when the authored command is a cvar toggle.
    bool setBindToggle(const QString &key, const QString &command);

    /// Drop every bind row for key token `key`. False when the key had no bind row.
    bool clearBind(const QString &key);

    /// Binding kind for key token `key`: "bind", "bindtoggle", or "" when unbound.
    QString bindingKind(const QString &key) const;

    /// Active binding kinds: lowercase key token -> "bind"/"bindtoggle", in step with
    /// binds(). Empty when the key is unbound.
    QVariantMap bindingKinds() const;

    /// Replace the LINE-GROUP of `row` with `segments` (docs/DESIGN-config-editor.md
    /// §5/§6): a group is the rows sharing the row's sourceLine (a parsed compound
    /// line), the rows sharing its groupId (a session compound insert), or the row
    /// alone. Surviving rows keep their line identity; extra segments insert as new
    /// group members, extra model rows are removed (their lines drop on save). Each
    /// segment carries its own args/type/commentTail; appOwned is set. False when the
    /// row is out of range or segments is empty.
    bool setRowGroup(int row, const QVector<cmd::Entry> &segments);

    /// Remove a row (Config Editor delete). The surgical save then drops its line.
    /// False when the row is out of range.
    bool deleteEntry(int row);

    /// Insert a session LINE of one or more segments (`;`-joined on save for >1,
    /// sharing a fresh groupId) whose save position is anchored to `anchorRow`'s file
    /// line (Config Editor insert-after-selected, §9). anchorRow < 0 appends at end of
    /// file. Returns the first new row index, -1 on refusal (empty segments).
    int insertLine(int anchorRow, const QVector<cmd::Entry> &segments);

    /// Define or update an alias (docs/DESIGN-config-editor.md §5.3, BindEditorDialog
    /// "Alias" tab). `body` is raw command text (`slot3; slot1`), stored quoted. The
    /// latest `alias <name>` row is rewritten in place and earlier definitions of the
    /// same name are dropped (a later definition wins in TF2's replay, so the others
    /// are dead weight - the same competing-row rule setBind applies). No definition
    /// yet: a session row anchored after the last alias line, else end of file (the
    /// bundled presets define aliases after their binds, so that is where they belong).
    /// An empty body removes the definition instead. False when the name is empty or
    /// the body is unchanged.
    bool setAlias(const QString &name, const QString &body);

    /// Names defined by `alias` rows, first-seen order, deduplicated (case-insensitive).
    QStringList aliasNames() const;

    /// { found, name, body } for the latest definition of `name` (case-insensitive);
    /// body is the raw unquoted text, "" for a bare `alias <name>` row.
    QVariantMap aliasInfo(const QString &name) const;

    /// One row as a QVariantMap { row, line, command, type, typeName, boundKey, warning }
    /// for the Config Editor's editor pane (it sits outside the list delegate, so it
    /// cannot read roles). Empty map when the row is out of range.
    Q_INVOKABLE QVariantMap rowInfo(int row) const;

    static QString typeName(cmd::CmdType type);

signals:
    /// Bind state changed: rows were edited, or a whole new command list was parsed.
    void bindsChanged();

private:
    /// Rows that bind or unbind key token `key`, in parse order.
    QVector<int> bindRows(const QString &key) const;

    struct BindScanResult {
        QVariantMap binds;
        QVariantMap bindingKinds;
        QHash<QString, QVector<int>> rowsByKey;
        /// Replay-order warnings (docs/DESIGN-config-editor.md §7): source row -> text.
        QHash<int, QString> warnings;
    };
    BindScanResult scanBinds() const;

    /// Lowercase key token a bind/bindtoggle row applies to, else "" (BoundKeyRole logic).
    QString boundKeyOf(const cmd::Entry &entry) const;

    /// Run scanBinds and refresh all three caches (binds, kinds, warnings).
    void ensureScan() const;

    /// Reset the derived caches and repaint the warning column. Every row mutation
    /// goes through this, so the keyboard, the Config Editor badges and `dirty` all
    /// stay in step.
    void invalidateDerived();

    QVector<cmd::Entry> m_entries;
    int m_nextGroupId = 1; ///< session compound identity (§5); reset on every reload
    mutable QVariantMap m_bindsCache;
    mutable bool m_bindsCacheValid = false;
    mutable QVariantMap m_bindingKindsCache;
    mutable bool m_bindingKindsCacheValid = false;
    mutable QHash<int, QString> m_warningsCache;
    mutable bool m_warningsCacheValid = false;
};

#endif // COMMANDLISTMODEL_H
