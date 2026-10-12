import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../components"
import "../../theme"

// Tab 0: every parsed CFG line, monospaced. Parity P15/P16/P38.
Panel {
    id: tab

    title: "Commands"

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: Theme.colorSurfaceSunken
        border.width: Theme.borderWidth
        border.color: Theme.colorBorder
        radius: Theme.radius
        clip: true

        ListView {
            id: commandList
            anchors.fill: parent
            anchors.margins: Theme.spacingTight
            clip: true
            reuseItems: true
            model: cfg.commandModel
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Text {
                id: line

                // Role "line" is args.join(" "), exactly what the widget list showed.
                required property string line

                width: commandList.width
                text: line
                color: Theme.colorText
                font.family: Theme.familyMono
                font.pointSize: Theme.sizeMono
                elide: Text.ElideRight

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }

        Tf2Label {
            anchors.centerIn: parent
            visible: commandList.count === 0
            text: "No config loaded."
            color: Theme.colorTextMuted
        }
    }
}
