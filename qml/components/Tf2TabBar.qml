pragma ComponentBehavior: Bound

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

    // W3: the strip joins the tab chain; Left/Right move and activate, and the
    // focused tab shows a ring (a11y step 2 of the manual pass).
    activeFocusOnTab: tabCount > 0

    // W4, fixed 2026-10-15 after the Narrator pass: the tabs below each announce as a
    // page tab, but a page tab is only announced inside a tab list - Qt's own TabBar
    // does exactly this pair (QQuickTabBar -> PageTabList, QQuickTabButton -> PageTab).
    // Without it Narrator read the strip as a nameless pane and skipped the tab names.
    Accessible.role: Accessible.PageTabList

    Keys.onLeftPressed: function (event) { control.moveCurrent(event, -1) }
    Keys.onRightPressed: function (event) { control.moveCurrent(event, 1) }

    function moveCurrent(event, delta) {
        if (tabCount === 0)
            return
        event.accepted = true
        activated((currentIndex + delta + tabCount) % tabCount)
    }

    implicitHeight: Theme.tabHeight
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
                /// True when the tab strip holds keyboard focus and this tab would
                /// activate with Return/Space - the focus ring's condition.
                readonly property bool isFocus: control.activeFocus
                                                && control.currentIndex === index

                implicitWidth: Math.max(Theme.tabMinWidth, tabLabel.implicitWidth + Theme.spacingSection)
                implicitHeight: Theme.tabHeight
                width: implicitWidth
                height: implicitHeight

                // W3 + W4: each tab announces as a page tab; Left/Right switch, and
                // Return activates the focused one.
                Accessible.role: Accessible.PageTab
                Accessible.name: tabItem.modelData
                Accessible.selected: tabItem.isCurrent
                Accessible.onPressAction: control.activated(tabItem.index)

                Rectangle {
                    anchors.fill: parent
                    radius: Theme.radius
                    color: tabItem.isCurrent ? Theme.colorSurface
                                             : hover.hovered ? Theme.colorSurfaceRaised
                                             : "transparent"
                    // Focus ring: 2 px accent border, same cue as Tf2Button (W1).
                    border.width: tabItem.isFocus ? 2 : Theme.borderWidth
                    border.color: tabItem.isFocus ? Theme.colorAccent : "transparent"

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
