pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "../theme"

// One key cap. All data comes from the keyTable C++ context property, so
// adding/removing keys is a data edit, not a UI edit (docs/legacy/ui-qml-rewrite.md
// section 10; the table moved from a QML singleton to C++, deviation D9).
Rectangle {
    id: cap

    /// Local handles for the context properties, so later accesses are qualified and
    /// do not each add a separate unqualified-access advisory.
    readonly property var cfgRef: cfg
    readonly property var tableRef: keyTable

    /// Key name into the key table (e.g. "esc", "bkspc", "kpplus").
    property string keyName: ""
    /// Command shown in the cap's bind overlay for the future "show what this key does"
    /// mode (D16). Now populated for bound caps; empty when unbound.
    property string bindText: cap.bound ? cap.boundCommand : ""

    readonly property var keyInfo: tableRef.keyData(keyName)
    readonly property bool bindable: tableRef.isBindable(keyName)
    readonly property int keyId: tableRef.keyId(keyName)
    readonly property string token: tableRef.keyToken(keyName)
    /// Label size override for caps whose strings run long (mouse caps).
    readonly property int labelPointSize: keyInfo !== null && keyInfo.fs !== undefined
                                          ? keyInfo.fs : Theme.sizeUi

    /// The command the loaded config binds to this cap's token, or "" when unbound.
    /// Read straight off the C++ bind map, so the cap repaints whenever a bind is added
    /// or removed (docs/legacy/ui-qml-rewrite.md D16).
    readonly property string boundCommand: {
        if (cap.token === "")
            return ""
        const command = cap.cfgRef.binds[cap.token.toLowerCase()]
        return command === undefined ? "" : command
    }
    readonly property bool bound: cap.boundCommand !== ""

    /// Emitted with the engine key id (see docs/legacy/ui-qml-rewrite.md section 7).
    signal activated(int keyId)

    implicitWidth: tableRef.keyWidth(keyName, Theme.capSize)
    implicitHeight: tableRef.keyHeight(keyName, Theme.capSize, Theme.capGap)

    /// W2: bindable caps join the tab chain and activate on Return/Space; the two
    /// non-bindable caps (no token, D16) stay out entirely.
    activeFocusOnTab: cap.bindable
    Keys.onReturnPressed: function (event) { cap.activateIfBindable(event) }
    Keys.onSpacePressed: function (event) { cap.activateIfBindable(event) }

    function activateIfBindable(event) {
        if (cap.bindable && cap.keyId >= 0) {
            cap.activated(cap.keyId)
            event.accepted = true
        }
    }

    /// Hover and press are the cap's own affordance: a cap that cannot be clicked never
    /// shows them, even when the config does have something bound to it.
    /// Hover and press are the cap's own affordance: a cap that cannot be clicked never
    /// shows them, even when the config does have something bound to it. Keyboard focus
    /// gets the same highlight as hover (W2).
    readonly property bool highlighted: cap.bindable && (mouse.containsMouse || cap.activeFocus)
    readonly property bool pressed: cap.bindable && mouse.pressed

    /// Cap fill for the hover/press states, split by bound and unbound so the colour
    /// binding below stays readable.
    readonly property color fillWhenBound: cap.pressed ? Theme.colorKeyCapBoundPressed
                                         : cap.highlighted ? Theme.colorKeyCapBoundHover
                                         : Theme.colorKeyCapBound
    readonly property color fillWhenUnbound: cap.pressed ? Theme.colorKeyCapPressed
                                           : cap.highlighted ? Theme.colorKeyCapHover
                                           : Theme.colorKeyCap

    radius: Theme.radius
    border.width: Theme.borderWidth
    border.color: cap.highlighted ? Theme.colorBorderStrong
                : bound ? Theme.colorKeyCapBoundBorder
                : Theme.colorKeyCapBorder
    // A bound key is burnt orange. The two caps the widget build could not bind keep
    // their dimmed fill even when bound - PrtScn and Menu both carry no token (D16).
    color: bound ? cap.fillWhenBound
         : bindable ? cap.fillWhenUnbound
         : Theme.colorKeyCapDisabled
    // The two non-bindable caps are dimmed as a whole (fill, border, label) - PrtScn
    // and Menu carry no token (D16). 0.65 keeps the label over the disabled fill at
    // ~5.4:1 (W5); at 0.55 it was ~2:1 with the real label token.
    opacity: bindable ? 1.0 : 0.65

    Text {
        id: capLabel
        anchors.fill: parent
        anchors.margins: Theme.spacingTight
        anchors.bottomMargin: cap.bindText !== "" ? cap.height * 0.34 : Theme.spacingTight
        text: tableRef.keyLabel(cap.keyName)
        color: Theme.colorKeyCapText
        font.family: Theme.familyButton
        font.pointSize: cap.labelPointSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        fontSizeMode: Text.Fit
        minimumPixelSize: 8
        elide: Text.ElideRight
    }

    // Bind overlay slot. Renders nothing while bindText is empty, which it always is:
    // a bound cap is painted, not annotated (D16). The plumbing stays for a later
    // "show me what this key does" mode.
    Rectangle {
        id: bindSlot
        visible: cap.bindText !== ""
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 3
        height: cap.height * 0.28
        radius: 2
        color: Theme.colorAccent

        Text {
            anchors.fill: parent
            anchors.margins: 1
            text: cap.bindText
            color: Theme.colorAccentText
            font.family: Theme.familyMono
            font.pointSize: Theme.sizeSmall
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: cap.bindable ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            if (cap.bindable && cap.keyId >= 0)
                cap.activated(cap.keyId)
        }
    }

    // Hover text: the table's static tooltip for un-bindable caps (P26), else
    // the cap's current bind line for anything the loaded config has bound.
    readonly property string tipText: {
        if (cap.keyInfo !== null && cap.keyInfo.tooltip !== undefined)
            return cap.keyInfo.tooltip
        if (cap.bound) {
            const kind = cap.cfgRef.bindingKinds[cap.token.toLowerCase()]
            const verb = kind === "bindtoggle" ? "bindtoggle" : "bind"
            return verb + ' "' + cap.token + '" "' + cap.boundCommand + '"'
        }
        return ""
    }
    // W7 companion of W2: the tooltip is also the focus cue for what the cap does.
    ToolTip.visible: (mouse.containsMouse || cap.activeFocus) && cap.tipText !== ""
    ToolTip.text: cap.tipText
    ToolTip.delay: 400

    // W4: the cap announces as a button with its key label; the bind line, when any,
    // is the description a screen reader reads after the name.
    Accessible.role: Accessible.Button
    Accessible.name: tableRef.keyLabel(cap.keyName)
    Accessible.description: cap.bound ? cap.tipText : ""
    Accessible.onPressAction: cap.activateIfBindable(null)
}
