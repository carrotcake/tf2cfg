import QtQuick
import QtQuick.Controls
import "pages"
import "theme"

// Application shell. One window with a page stack, replacing the widget build's two
// QMainWindow instances that hid/showed each other (deviation D1 in
// docs/legacy/ui-qml-rewrite.md).
//
// Window size: both screens open at the same size and switching pages does not resize
// the window. The start screen is fixed; the editor is resizable from that same size up
// (deviation D2).
ApplicationWindow {
    id: window

    visible: true
    color: Theme.colorBg
    width: Theme.windowWidth
    height: Theme.windowHeight
    minimumWidth: Theme.windowWidth
    minimumHeight: Theme.windowHeight
    // Qt Quick's Window has no `resizable` flag: minimum == maximum is what pins it.
    // The start screen is fixed; goToEditor() releases the maximums and keeps the
    // minimums as the editor's floor.
    maximumWidth: Theme.windowWidth
    maximumHeight: Theme.windowHeight
    title: pageStack.currentItem !== null ? pageStack.currentItem.pageTitle : "TF2 Config Manager"

    function goToEditor() {
        pageStack.replace(editorComponent)
        // No width/height change: the editor opens at the size the start screen had.
        // The maximums go to the desktop so the editor can be dragged larger, onto a
        // second monitor included; the minimums stay at the shared default.
        window.maximumWidth = Screen.desktopAvailableWidth
        window.maximumHeight = Screen.desktopAvailableHeight
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
