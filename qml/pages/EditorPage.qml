import QtQuick
import QtQuick.Layouts
import "../components"
import "../dialogs"
import "../theme"
import "tabs"
pragma ComponentBehavior: Bound

// Editor screen. Parity P13-P22 in docs/legacy/ui-qml-rewrite.md.
Item {
    id: page

    /// Status-strip hint, mirroring the widget build's status tips (P20).
    property string statusHint: ""

    function tabTitle(index) {
        return tabBar.titleAt(index)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // -- tabs (P14/P15/P17/P18) -------------------------------------------
        Tf2TabBar {
            id: tabBar
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacing
            Layout.rightMargin: Theme.spacing
            Layout.topMargin: Theme.spacing
            model: ["Command Overview", "Config Editor", "Edit Binds"]
            onActivated: function (index) { tabBar.currentIndex = index }
        }

        StackLayout {
            id: tabStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.spacing
            currentIndex: tabBar.currentIndex

            CommandOverviewTab {}
            ConfigEditorTab {
                // §1: bind rows redirect to Edit Binds - switch tabs, then open the
                // key's dialog (P29 parity).
                onBindActivated: function (keyToken) {
                    tabBar.currentIndex = 2
                    bindDialog.openForKey(keyToken)
                }
            }
            BindEditorTab {
                onKeyActivated: function (keyId, token) {
                    // Parity P29: clicking a cap opens the modal dialog prefilled with
                    // bind "<token>".
                    bindDialog.openForKey(token)
                }
            }
        }

        // -- status strip (P20/P22) -------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: Theme.statusBarHeight
            color: Theme.colorSurface

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: Theme.borderWidth
                color: Theme.colorBorder
            }

            Tf2Label {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingWide
                anchors.rightMargin: Theme.spacingWide
                verticalAlignment: Text.AlignVCenter
                text: page.statusHint
                color: Theme.colorTextMuted
                elide: Text.ElideRight
            }
        }

        // -- prev / next (P19) -------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.spacing
            spacing: Theme.spacing

            Tf2Button {
                text: "Prev"
                flat: true
                enabled: tabBar.currentIndex > 0
                onHoveredChanged: page.statusHint = hovered ? page.tabTitle(tabBar.currentIndex - 1) : ""
                onClicked: tabBar.currentIndex = Math.max(0, tabBar.currentIndex - 1)
            }

            Tf2Button {
                text: "Next"
                flat: true
                enabled: tabBar.currentIndex < tabBar.tabCount - 1
                onHoveredChanged: page.statusHint = hovered ? page.tabTitle(tabBar.currentIndex + 1) : ""
                onClicked: tabBar.currentIndex = Math.min(tabBar.tabCount - 1, tabBar.currentIndex + 1)
            }

            Item { Layout.fillWidth: true }
        }
    }

    // Parity P30-P36: modal bind editor, opened from a key cap click.
    BindEditorDialog {
        id: bindDialog
    }
}
