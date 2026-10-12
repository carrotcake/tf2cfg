import QtQuick
import QtQuick.Controls
import "../theme"

// Themed combo box with placeholder support (currentIndex === -1 shows placeholderText,
// matching the widget build's "< Choose a Preset >" behaviour).
ComboBox {
    id: control

    property string placeholderText: ""

    implicitHeight: Theme.controlHeight
    implicitWidth: 200
    hoverEnabled: true
    font.family: Theme.familyUi
    font.pointSize: Theme.sizeUi
    displayText: (currentIndex >= 0 && currentText.length > 0) ? currentText : placeholderText

    contentItem: Text {
        leftPadding: Theme.spacing
        rightPadding: Theme.spacing * 3
        text: control.displayText
        font: control.font
        color: control.currentIndex >= 0 ? Theme.colorText : Theme.colorTextMuted
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Text {
        x: control.width - width - Theme.spacing
        y: (control.height - height) / 2
        text: "\u25BE"
        color: Theme.colorTextMuted
        font.pointSize: Theme.sizeUi
    }

    background: Rectangle {
        color: Theme.colorSurfaceSunken
        radius: Theme.radius
        border.width: Theme.borderWidth
        border.color: control.hovered || control.activeFocus ? Theme.colorBorderStrong : Theme.colorBorder
    }

    delegate: ItemDelegate {
        id: item

        required property var model
        required property int index

        width: control.width
        text: control.textRole !== "" && model[control.textRole] !== undefined ? model[control.textRole] : model
        font.family: Theme.familyUi
        font.pointSize: Theme.sizeUi
        highlighted: control.highlightedIndex === index

        contentItem: Text {
            text: item.text
            font: item.font
            color: Theme.colorText
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            // Highlighted row is the lighter surface, the list body the darker one.
            // (It used to be the other way round, which read as "selected = recessed"
            // on a dark theme.)
            color: item.highlighted ? Theme.colorSurfaceRaised : "transparent"
        }
    }

    popup: Popup {
        y: control.height
        width: control.width
        implicitHeight: Math.min(contentItem.implicitHeight + 2, 300)
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }

        background: Rectangle {
            color: Theme.colorSurfaceSunken
            border.width: Theme.borderWidth
            border.color: Theme.colorBorderStrong
            radius: Theme.radius
        }
    }
}
