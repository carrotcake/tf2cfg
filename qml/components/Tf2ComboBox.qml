pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "../theme"

// Themed combo box with placeholder support (currentIndex === -1 shows placeholderText,
// matching the widget build's "< Choose a Preset >" behaviour).
ComboBox {
    id: control

    property string placeholderText: ""

    implicitHeight: Theme.controlHeight
    implicitWidth: Theme.comboDefaultWidth
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

    // Hit zone for the drop-down indicator: an editable contentItem swallows
    // presses near the right edge, so the icon area gets its own click target.
    MouseArea {
        z: 2
        // Follows the combo's enabled state: a disabled combo must not open its popup.
        enabled: control.enabled
        x: control.width - width
        width: Theme.controlHeight
        height: control.height
        cursorShape: Qt.PointingHandCursor
        onClicked: control.popup.opened ? control.popup.close() : control.popup.open()
    }

    background: Rectangle {
        color: Theme.colorSurfaceSunken
        radius: Theme.radius
        // Hover is a subtle line; keyboard focus gets the app-wide 2 px accent ring
        // (W1/W3 consistency). Hover must not masquerade as focus.
        border.width: control.activeFocus ? 2 : Theme.borderWidth
        border.color: control.activeFocus ? Theme.colorAccent
                                          : control.hovered ? Theme.colorBorderStrong
                                          : Theme.colorBorder
    }

    delegate: ItemDelegate {
        id: item

        required property var model
        required property int index

        width: control.width
        // C++ models resolve roles through the model object; plain JS arrays wrap
        // their element in modelData, where a bare `model` stringifies to nothing.
        text: {
            if (control.textRole !== "") {
                const v = model[control.textRole]
                if (v !== undefined)
                    return String(v)
            }
            return model.modelData !== undefined ? String(model.modelData) : String(model)
        }
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

    // Opt-in popup grouping: when set to a role name of the model, the popup lists
    // rows under section headers for that role (the model must sort by it, e.g. the
    // command picker's context-then-name order). Empty = flat list, as everywhere.
    property string sectionRole: ""

    popup: Popup {
        y: control.height
        width: control.width
        implicitHeight: Math.min(contentItem.implicitHeight + 2 * padding, Theme.popupMaxHeight)
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
            section.property: control.sectionRole
            section.delegate: ItemDelegate {
                visible: control.sectionRole !== "" && section !== ""
                height: visible ? implicitHeight : 0
                text: section
                highlighted: false
                hoverEnabled: false
                font.family: Theme.familyUi
                font.pointSize: Theme.sizeUi
                font.capitalization: Font.SmallCaps
                // Header, not a row: muted text on the sunken surface, inset border.
                leftPadding: Theme.spacing
                background: Rectangle {
                    color: Theme.colorSurfaceSunken
                    Rectangle {
                        anchors.fill: parent
                        color: Theme.colorTextMuted
                        opacity: 0.15
                        height: 1
                        anchors.bottom: parent.bottom
                    }
                }
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: Theme.colorTextMuted
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        background: Rectangle {
            color: Theme.colorSurfaceSunken
            border.width: Theme.borderWidth
            border.color: Theme.colorBorderStrong
            radius: Theme.radius
        }
    }
}
