import QtQuick
import QtQuick.Controls
import "../theme"
pragma ComponentBehavior: Bound

// Game-style push button. Primary (accent filled) by default, secondary with flat: true.
Button {
    id: control

    implicitHeight: Theme.controlHeight
    implicitWidth: Math.max(96, contentItem.implicitWidth + Theme.spacingWide * 2)
    hoverEnabled: true
    font.family: Theme.familyButton
    font.pointSize: Theme.sizeButton

    contentItem: Text {
        text: control.text
        font: control.font
        color: !control.enabled ? Theme.colorTextDisabled
                                : control.flat ? Theme.colorText
                                : Theme.colorAccentText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: Theme.radius
        color: !control.enabled ? Theme.colorSurfaceRaised
                                : control.pressed ? (control.flat ? Theme.colorSurfaceRaised : Theme.colorAccentPressed)
                                : control.hovered ? (control.flat ? Theme.colorSurfaceRaised : Theme.colorAccentHover)
                                : (control.flat ? "transparent" : Theme.colorAccent)
        border.width: control.activeFocus ? 2 : Theme.borderWidth
        border.color: !control.enabled ? Theme.colorBorder
                                       : control.activeFocus ? Theme.colorAccent
                                       : control.flat ? Theme.colorBorderStrong
                                       : Theme.colorAccentHover
    }
}
