import QtQuick
import QtQuick.Controls
import "../theme"

// One key cap. All data comes from the KeyTable singleton, so adding/removing keys is
// a data edit, not a UI edit (docs/ui-qml-rewrite.md section 10).
Rectangle {
    id: cap

    /// Key name into KeyTable (e.g. "esc", "bkspc", "kpplus").
    property string keyName: ""
    /// Reserved for the bind overlay: the token this cap is bound to, empty today.
    property string bindText: ""

    readonly property var keyInfo: KeyTable.keyData(keyName)
    readonly property bool bindable: KeyTable.isBindable(keyName)
    readonly property int keyId: KeyTable.keyId(keyName)
    readonly property string token: KeyTable.keyToken(keyName)

    /// Emitted with the engine key id (see docs/ui-qml-rewrite.md section 7).
    signal activated(int keyId)

    implicitWidth: KeyTable.keyWidth(keyName)
    implicitHeight: KeyTable.keyHeight(keyName)

    radius: Theme.radius
    border.width: Theme.borderWidth
    border.color: mouse.containsMouse && bindable ? Theme.colorBorderStrong : Theme.colorKeyCapBorder
    color: !bindable ? Theme.colorKeyCapDisabled
                    : mouse.pressed ? Theme.colorKeyCapPressed
                    : mouse.containsMouse ? Theme.colorKeyCapHover
                    : Theme.colorKeyCap
    opacity: bindable ? 1.0 : 0.55

    Text {
        id: capLabel
        anchors.fill: parent
        anchors.margins: Theme.spacingTight
        anchors.bottomMargin: cap.bindText !== "" ? cap.height * 0.34 : Theme.spacingTight
        text: KeyTable.keyLabel(cap.keyName)
        color: cap.bindable ? Theme.colorKeyCapText : Theme.colorTextDisabled
        font.family: Theme.familyButton
        font.pointSize: Theme.sizeUi
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        fontSizeMode: Text.Fit
        minimumPixelSize: 8
        elide: Text.ElideRight
    }

    // Bind overlay slot. Renders nothing while bindText is empty, which it always is
    // until the bind engine lands; the plumbing (roles, property, geometry) is done.
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

    // Parity P26: the two un-bindable caps explain themselves on hover.
    ToolTip.visible: mouse.containsMouse && cap.keyInfo !== null && cap.keyInfo.tooltip !== undefined
    ToolTip.text: cap.keyInfo !== null && cap.keyInfo.tooltip !== undefined ? cap.keyInfo.tooltip : ""
    ToolTip.delay: 400
}
