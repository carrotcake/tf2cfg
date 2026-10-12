import QtQuick
import "../../keyboard"
import "../../theme"

// Tab 2: the keyboard. Parity P18/P23-P28.
Item {
    id: tab

    /// Forwarded from a key cap click; EditorPage owns the bind dialog (P29).
    signal keyActivated(int keyId, string token)

    KeyboardPanel {
        anchors.fill: parent
        onKeyActivated: function (keyId, token) { tab.keyActivated(keyId, token) }
    }
}
