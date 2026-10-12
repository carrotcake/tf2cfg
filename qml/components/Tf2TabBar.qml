import QtQuick
import "../theme"

// Themed tab strip.
//
// Deliberately not QtQuick.Controls.TabBar: this is a small, fully styled control whose
// API (model / tabCount / currentIndex / titleAt) covers exactly what the app needs, and
// it keeps the look identical between the editor tabs and the bind dialog tabs.
Item {
    id: control

    /// Array of tab titles.
    property var model: []
    property int currentIndex: 0
    readonly property int tabCount: model ? model.length : 0

    signal activated(int index)

    implicitHeight: 34
    implicitWidth: tabRow.implicitWidth

    function titleAt(index) {
        return (model && index >= 0 && index < model.length) ? model[index] : ""
    }

    Row {
        id: tabRow
        anchors.left: parent.left
        anchors.top: parent.top
        spacing: 2

        Repeater {
            model: control.model

            delegate: Item {
                id: tabItem

                required property int index
                required property var modelData

                readonly property bool isCurrent: control.currentIndex === index

                implicitWidth: Math.max(140, tabLabel.implicitWidth + Theme.spacingSection)
                implicitHeight: 34
                width: implicitWidth
                height: implicitHeight

                Rectangle {
                    anchors.fill: parent
                    radius: Theme.radius
                    color: tabItem.isCurrent ? Theme.colorSurface
                                             : hover.hovered ? Theme.colorSurfaceRaised
                                             : "transparent"

                    Text {
                        id: tabLabel
                        anchors.fill: parent
                        anchors.margins: Theme.spacingTight
                        text: tabItem.modelData
                        color: tabItem.isCurrent ? Theme.colorAccent
                                                 : hover.hovered ? Theme.colorText
                                                 : Theme.colorTextMuted
                        font.family: Theme.familyButton
                        font.pointSize: Theme.sizeButton
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        visible: tabItem.isCurrent
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 2
                        color: Theme.colorAccent
                    }
                }

                HoverHandler { id: hover }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: control.activated(tabItem.index)
                }
            }
        }
    }

    // Rule under the whole strip.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: Theme.borderWidth
        color: Theme.colorBorder
    }
}
