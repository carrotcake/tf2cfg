#ifndef KEYTABLE_H
#define KEYTABLE_H

#include <QHash>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

/**
 * The key table, ported from the retired QML singleton `KeyTable.qml`
 * (deviation D9: static UI data belongs in C++, not in a QML singleton).
 *
 * One entry per physical cap, plus the row layout used to draw the keyboard,
 * the mouse block and the controller block.
 *
 * `id` and `token` are a parity contract with the retired widget build
 * (keybform.h KeyId/KEYCODES). Do not renumber and do not "fix" token spellings -
 * they are the engine's own strings and the bind engine depends on both.
 */
class KeyTable : public QObject {
    Q_OBJECT
    /// Layout, mirroring the widget build's frames (keybform.ui). A null entry in
    /// `middleRows` is the blank slot above the arrow keys.
    Q_PROPERTY(QVariantList fnRowGroups READ fnRowGroups CONSTANT)
    Q_PROPERTY(QVariantList mainRows READ mainRows CONSTANT)
    Q_PROPERTY(QVariantList middleRows READ middleRows CONSTANT)
    Q_PROPERTY(QVariantList numpadMain READ numpadMain CONSTANT)
    Q_PROPERTY(QVariantList numpadRight READ numpadRight CONSTANT)
    /// Mouse buttons/wheel cells (name/x/y/w/h), an over-the-shoulder mouse
    /// rotated 90 degrees counter-clockwise.
    Q_PROPERTY(QVariantList mouseCells READ mouseCells CONSTANT)
    Q_PROPERTY(qreal mouseWidth READ mouseWidth CONSTANT)
    Q_PROPERTY(qreal mouseHeight READ mouseHeight CONSTANT)
    /// Controller cells (name/x/y/w/h), an Xbox 360 pad seen from above.
    Q_PROPERTY(QVariantList controllerCells READ controllerCells CONSTANT)
    Q_PROPERTY(qreal controllerWidth READ controllerWidth CONSTANT)
    Q_PROPERTY(qreal controllerHeight READ controllerHeight CONSTANT)

public:
    explicit KeyTable(QObject *parent = nullptr);

    /// Per-cap data for `name` (id/token/label/w/rows/fs/bindable/tooltip), or an
    /// empty map for an unknown name.
    Q_INVOKABLE QVariantMap keyData(const QString &name) const;
    /// Whether a cap can be bound to an action.
    Q_INVOKABLE bool isBindable(const QString &name) const;
    /// Cap width in px: the cap's own width, or `defaultW` (the caller's
    /// Theme.capSize) for standard caps.
    Q_INVOKABLE qreal keyWidth(const QString &name, qreal defaultW) const;
    /// Cap height in px, honouring multi-row keys (numpad + and Enter only):
    /// `rows * capSize + (rows - 1) * capGap` with the caller's Theme values.
    Q_INVOKABLE qreal keyHeight(const QString &name, qreal capSize, qreal capGap) const;
    /// Display label of a cap (falls back to the name itself).
    Q_INVOKABLE QString keyLabel(const QString &name) const;
    /// TF2 key token, or empty for the un-bindable caps.
    Q_INVOKABLE QString keyToken(const QString &name) const;
    /// Engine key id, or -1 for the un-bindable caps.
    Q_INVOKABLE int keyId(const QString &name) const;

    /// Reverse lookups, for the bind engine / keyboard overlay.
    Q_INVOKABLE QString tokenForId(int id) const;
    Q_INVOKABLE QString nameForToken(const QString &token) const;

    QVariantList fnRowGroups() const { return m_fnRowGroups; }
    QVariantList mainRows() const { return m_mainRows; }
    QVariantList middleRows() const { return m_middleRows; }
    QVariantList numpadMain() const { return m_numpadMain; }
    QVariantList numpadRight() const { return m_numpadRight; }
    QVariantList mouseCells() const { return m_mouseCells; }
    QVariantList controllerCells() const { return m_controllerCells; }
    qreal mouseWidth() const { return m_mouseWidth; }
    qreal mouseHeight() const { return m_mouseHeight; }
    qreal controllerWidth() const { return m_controllerWidth; }
    qreal controllerHeight() const { return m_controllerHeight; }

private:
    void add(const QString &name, int id, const QString &token, const QString &label,
             qreal w = 0, int rows = 0, int fs = 0, const QString &tooltip = QString());
    static qreal blockExtent(const QVariantList &cells, const char *start,
                             const char *span);

    struct Cap {
        int id;
        QString token;
        QString label;
        qreal w;     // 0 = default (Theme.capSize, resolved in QML)
        int rows;    // 0 = single row
        int fs;      // 0 = default label size
        bool bindable;
        QString tooltip;
    };
    QHash<QString, Cap> m_caps;

    QVariantList m_fnRowGroups, m_mainRows, m_middleRows, m_numpadMain, m_numpadRight;
    QVariantList m_mouseCells, m_controllerCells;
    qreal m_mouseWidth, m_mouseHeight, m_controllerWidth, m_controllerHeight;
};

#endif // KEYTABLE_H