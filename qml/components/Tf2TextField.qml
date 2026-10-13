import QtQuick
import QtQuick.Controls
import "../theme"
pragma ComponentBehavior: Bound

// Themed single-line field (config path, existing bind, future argument editors).
TextField {
    id: control

    implicitHeight: Theme.controlHeight
    leftPadding: Theme.spacing
    rightPadding: Theme.spacing
    color: Theme.colorText
    placeholderTextColor: Theme.colorTextMuted
    selectionColor: Theme.colorAccent
    selectedTextColor: Theme.colorAccentText
    font.family: Theme.familyUi
    font.pointSize: Theme.sizeUi
    hoverEnabled: true
    selectByMouse: true

    background: Rectangle {
        color: Theme.colorSurfaceSunken
        radius: Theme.radius
        border.width: Theme.borderWidth
        border.color: control.activeFocus ? Theme.colorAccent
                                          : control.hovered ? Theme.colorBorderStrong
                                          : Theme.colorBorder
    }
}
