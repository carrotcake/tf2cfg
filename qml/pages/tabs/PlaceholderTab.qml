import QtQuick
import QtQuick.Layouts
import "../../components"
import "../../theme"

// Tab 1: reserved. The widget build had an empty placeholder here (parity P17); the
// renaming of its joke label is deviation D6 in docs/ui-qml-rewrite.md.
Panel {
    id: tab

    Tf2Label {
        Layout.fillWidth: true
        Layout.fillHeight: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: "Nothing here yet."
        color: Theme.colorTextMuted
    }
}
