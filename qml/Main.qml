import QtQuick
import QtQuick.Controls
import "pages"
import "theme"

// Application shell. One window with a page stack, replacing the widget build's two
// QMainWindow instances that hid/showed each other (deviation D1 in
// docs/legacy/ui-qml-rewrite.md). Per-page layout and window sizes are unchanged (D2).
ApplicationWindow {
    id: window

    visible: true
    color: Theme.colorBg
    width: Theme.startWidth
    height: Theme.startHeight
    minimumWidth: Theme.startWidth
    minimumHeight: Theme.startHeight
    title: pageStack.currentItem !== null ? pageStack.currentItem.pageTitle : "TF2 Config Manager"

    function goToEditor() {
        pageStack.replace(editorComponent)
        window.minimumWidth = Theme.editorMinWidth
        window.minimumHeight = Theme.editorMinHeight
        window.width = Theme.editorWidth
        window.height = Theme.editorHeight
    }

    // Parity P21: File/Edit/View/Help exist but carry no actions, and only the editor
    // screen shows them.
    menuBar: MenuBar {
        visible: pageStack.depth > 1

        Menu { title: "File" }
        Menu { title: "Edit" }
        Menu { title: "View" }
        Menu { title: "Help" }
    }

    StackView {
        id: pageStack
        anchors.fill: parent
        initialItem: startComponent
    }

    Component {
        id: startComponent

        StartPage {
            onLoaded: window.goToEditor()
        }
    }

    Component {
        id: editorComponent

        EditorPage {}
    }
}
