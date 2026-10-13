pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "../theme"

// One key cap. All data comes from the KeyTable singleton, so adding/removing keys is
// a data edit, not a UI edit (docs/legacy/ui-qml-rewrite.md section 10).
Rectangle {
    id: cap

    /// Local handle for the context property, so later accesses are qualified and do
    /// not each add a separate unqualified-access advisory.
    readonly property var cfgRef: cfg

    /// Key name into KeyTable (e.g. "esc", "bkspc", "kpplus").
    property string keyName: ""
    /// Command shown in the cap's bind overlay for the future "show what this key does"
    /// mode (D16). Now populated for bound caps; empty when unbound.
    property string bindText: cap.bound ? cap.boundCommand : ""

    readonly property var keyInfo: KeyTable.keyData(keyName)
    readonly property bool bindable: KeyTable.isBindable(keyName)
    readonly property int keyId: KeyTable.keyId(keyName)
    readonly property string token: KeyTable.keyToken(keyName)
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

    implicitWidth: KeyTable.keyWidth(keyName)
    implicitHeight: KeyTable.keyHeight(keyName)

    /// Hover and press are the cap's own affordance: a cap that cannot be clicked never
    /// shows them, even when the config does have something bound to it.
    readonly property bool highlighted: cap.bindable && mouse.containsMouse
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
    opacity: bindable ? 1.0 : 0.55

    Text {
        id: capLabel
        anchors.fill: parent
        anchors.margins: Theme.spacingTight
        anchors.bottomMargin: cap.bindText !== "" ? cap.height * 0.34 : Theme.spacingTight
        text: KeyTable.keyLabel(cap.keyName)
        color: cap.bindable ? Theme.colorKeyCapText : Theme.colorTextDisabled
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
    ToolTip.visible: mouse.containsMouse && cap.tipText !== ""
    ToolTip.text: cap.tipText
    ToolTip.delay: 400
}
