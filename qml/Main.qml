pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
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
    // Titles live here rather than on the pages: reading pageStack.currentItem.pageTitle
    // is a member lookup on QQuickItem, which has no such property (the one real qmllint
    // warning in the tree).
    title: "TF2 Config Manager"

    /// True once the editor page is up. StackView.replace() keeps depth at 1, so
    /// pageStack.depth cannot tell the pages apart - this flag is what the menu bar
    /// hangs its visibility on.
    property bool editorActive: false

    function goToEditor() {
        editorActive = true
        pageStack.replace(editorComponent)
        // No width/height change: the editor opens at the size the start screen had.
        // The maximums go to the desktop so the editor can be dragged larger, onto a
        // second monitor included; the minimums stay at the shared default.
        window.maximumWidth = Screen.desktopAvailableWidth
        window.maximumHeight = Screen.desktopAvailableHeight
        window.title = "Team Fortress 2 Config Editor"
    }

    // Parity P21: File/Edit/View/Help, only on the editor screen. The File menu's save
    // pair is the first working actions the menus have carried (deviation D20).
    menuBar: MenuBar {
        visible: editorActive

        Menu {
            title: "File"

            Action {
                text: qsTr("Save")
                shortcut: "Ctrl+S"
                enabled: cfg.dirty && cfg.canSave() && !cfg.loading
                onTriggered: window.save()
            }
            Action {
                text: qsTr("Save As...")
                shortcut: "Ctrl+Shift+S"
                enabled: cfg.loaded && !cfg.loading
                onTriggered: saveAsDialog.open()
            }
        }
        Menu { title: "Edit" }
        Menu { title: "View" }
        Menu { title: "Help" }
    }

    // The one confirm the save flow asks for: the file changed on disk since it was
    // loaded. Shows both stamps so the user can tell whose change is whose.
    Dialog {
        id: confirmSaveDialog
        modal: true
        closePolicy: Popup.CloseOnEscape
        anchors.centerIn: parent
        width: 460
        title: qsTr("File changed on disk")
        standardButtons: Dialog.Save | Dialog.Cancel
        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("%1 was changed by something else since it was loaded.\n\n"
                      + "Saving overwrites that version with this one (a %1.bak copy of "
                      + "the disk version is kept either way).").arg(cfg.cfgPath)
        }
        onAccepted: window.saveConfirmed()
    }

    FileDialog {
        id: saveAsDialog
        title: qsTr("Save Config As")
        fileMode: FileDialog.SaveFile
        nameFilters: ["Source Config File (*.cfg)"]
        defaultSuffix: "cfg"
        onAccepted: {
            const path = window.localPath(selectedFile)
            if (!cfg.saveCfgAs(path))
                saveError.show(cfg.lastError, true)
        }
    }

    function localPath(url) {
        // Same conversion StartPage does for Browse; kept here so the save dialog needs
        // no import of the start page.
        let s = url.toString()
        if (s.indexOf("file:") === 0) {
            s = s.replace(/^file:\/\//, "")
            try {
                s = decodeURIComponent(s)
            } catch (e) {
            }
            if (/^\/[A-Za-z]:/.test(s))
                s = s.slice(1)
        }
        return s
    }

    function save() {
        if (cfg.needsConfirm()) {
            confirmSaveDialog.open()
            return
        }
        saveConfirmed()
    }

    function saveConfirmed() {
        if (!cfg.saveCfg()) {
            saveError.show(cfg.lastError, true)
            return
        }
    }

    // Success feedback comes from the controller's saved() signal, so Save and Save As
    // share one path.
    Connections {
        target: cfg
        function onSaved(path) {
            saveError.show("Saved " + path, false)
        }
    }

    // One transient strip for save feedback: the reason a save was refused, or a brief
    // confirmation that it landed. Sits above the status strip and fades like the
    // keyboard's zoom badge.
    Rectangle {
        id: saveError
        parent: Overlay.overlay
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.spacingSection
        width: message.implicitWidth + Theme.spacingWide * 2
        height: message.implicitHeight + Theme.spacing
        radius: Theme.radius
        color: isError ? Theme.colorSurfaceRaised : Theme.colorSurface
        border.width: Theme.borderWidth
        border.color: isError ? Theme.colorRed : Theme.colorBorderStrong
        opacity: visibleCount > 0 ? 1 : 0
        visible: opacity > 0

        property bool isError: false
        property int visibleCount: 0

        function show(text, error) {
            message.text = text
            isError = error !== false
            visibleCount++
            fade.restart()
        }

        Text {
            id: message
            anchors.centerIn: parent
            color: saveError.isError ? Theme.colorRed : Theme.colorText
            font.family: Theme.familyUi
            font.pointSize: Theme.sizeUi
        }

        Timer {
            id: fade
            interval: 2600
            onTriggered: {
                saveError.visibleCount = Math.max(0, saveError.visibleCount - 1)
            }
        }
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
