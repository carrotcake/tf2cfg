pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "../theme"

// The keyboard. Four clusters, same arrangement as the widget build
// (docs/legacy/ui-qml-rewrite.md section 8): function row, main block, middle cluster, numpad.
// Everything is data-driven from KeyTable.
//
// Usability additions over the widget build (section 13): the panel scrolls in both
// directions when the window is too small for it, and the keyboard alone can be
// zoomed with Ctrl+wheel or Ctrl+plus/minus. Neither changes any layout value.
Item {
    id: panel

    /// Emitted when a bindable cap is clicked.
    signal keyActivated(int keyId, string token)

    /// Zoom of the keyboard (caps and their click areas) only. 1.0 is design size.
    property real zoom: 1.0
    readonly property real zoomMin: 0.5
    readonly property real zoomMax: 2.0
    readonly property real zoomStep: 0.1

    /// Padding around the keyboard inside the flickable content. The extra room over
    /// Theme.spacingWide keeps an overlay scroll bar off the outer caps.
    readonly property real contentMargin: Theme.spacingWide + 12

    /// Width of the widest main-block row; the other rows are stretched to match.
    readonly property real mainRowWidth: {
        let widest = 0
        const rows = KeyTable.mainRows
        for (let i = 0; i < rows.length; ++i) {
            const w = rowWidth(rows[i])
            if (w > widest)
                widest = w
        }
        return widest
    }
    readonly property real middleClusterWidth: Theme.capSize * 3 + Theme.capGap * 2

    /// Natural width of a row of caps.
    function rowWidth(names) {
        let w = 0
        for (let i = 0; i < names.length; ++i)
            w += KeyTable.keyWidth(names[i])
        if (names.length > 1)
            w += Theme.capGap * (names.length - 1)
        return w
    }

    /// Gap that stretches a main row to mainRowWidth, the way the widget's
    /// QHBoxLayouts stretched to the frame. Keeps the block rectangular.
    function mainRowSpacing(names) {
        if (names.length < 2)
            return Theme.capGap
        return Theme.capGap + (mainRowWidth - rowWidth(names)) / (names.length - 1)
    }

    /// X of a function-row group. Groups before the last sit at their natural
    /// cumulative spots; the last (PrtScn/ScrLk/Pause) is centered over the
    /// middle cluster below it, which starts at mainRowWidth + 2 cluster gaps.
    function fnGroupX(index) {
        const groups = KeyTable.fnRowGroups
        if (index === groups.length - 1)
            return mainRowWidth + Theme.capClusterGap * 2
        let x = 0
        for (let i = 0; i < index; ++i)
            x += rowWidth(groups[i]) + Theme.capClusterGap
        return x
    }

    /// Set the zoom, clamped to [zoomMin, zoomMax]. The viewport centre is kept over
    /// the same key, so zooming does not throw the panel off to a corner.
    function setZoom(value) {
        const clamped = Math.max(zoomMin, Math.min(zoomMax, value))
        if (Math.abs(clamped - zoom) < 0.0001)
            return

        const centreX = (flick.contentX + flick.width / 2 - keyboard.x) / zoom
        const centreY = (flick.contentY + flick.height / 2 - keyboard.y) / zoom

        zoom = clamped

        // The keyboard bindings above have already re-evaluated for the new zoom.
        // Flickable does not clamp assigned contentX/Y, so clamp to its own range.
        flick.contentX = Math.max(0, Math.min(keyboard.x + centreX * zoom - flick.width / 2,
                                              flick.contentWidth - flick.width))
        flick.contentY = Math.max(0, Math.min(keyboard.y + centreY * zoom - flick.height / 2,
                                              flick.contentHeight - flick.height))

        zoomBadge.opacity = 1
        zoomHint.restart()
    }

    function zoomBy(delta) {
        setZoom(zoom + delta)
    }

    /// Ctrl+0.
    function resetZoom() {
        setZoom(1.0)
    }

    // W6: arrow keys scroll the keyboard panel to the outer caps. The focus scope
    // forwards arrows here only when nothing else consumed them; KeyNavigation on the
    // caps would fight the strip's own arrow handling, so scrolling is global to the
    // panel instead of per-cap.
    Keys.onPressed: function (event) {
        const step = 60
        switch (event.key) {
        case Qt.Key_Left:
            flick.contentX = Math.max(0, flick.contentX - step)
            break
        case Qt.Key_Right:
            flick.contentX = Math.min(flick.contentX + step, flick.contentWidth - flick.width)
            break
        case Qt.Key_Up:
            flick.contentY = Math.max(0, flick.contentY - step)
            break
        case Qt.Key_Down:
            flick.contentY = Math.min(flick.contentY + step, flick.contentHeight - flick.height)
            break
        default:
            return
        }
        event.accepted = true
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: Math.max(width, keyboard.width * panel.zoom + panel.contentMargin * 2)
        contentHeight: Math.max(height, keyboard.height * panel.zoom + panel.contentMargin * 2)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        // Deterministic bars: shown only when the keyboard does not fit, and then
        // permanently visible (AsNeeded fades them out again when idle, which hides
        // the only affordance for reaching the outer keys).
        ScrollBar.vertical: ScrollBar {
            id: vbar
            policy: flick.contentHeight > flick.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        }
        ScrollBar.horizontal: ScrollBar {
            id: hbar
            policy: flick.contentWidth > flick.width ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        }

        // Ctrl+wheel zooms the keyboard; a plain wheel still scrolls (Flickable's own
        // wheel handling runs whenever this handler does not accept the event).
        WheelHandler {
            acceptedModifiers: Qt.ControlModifier
            onWheel: function (event) { panel.zoomBy(event.angleDelta.y / 120 * panel.zoomStep) }
        }

        Column {
            id: keyboard
            transformOrigin: Item.TopLeft
            scale: panel.zoom
            x: (flick.contentWidth - width * panel.zoom) / 2
            y: panel.contentMargin
            spacing: Theme.capGap

            // -- function row --------------------------------------------------
            // Plain Item with computed x per group, so the last group (PrtScn,
            // ScrLk, Pause) can center over the middle cluster below it instead
            // of sitting at its natural cumulative offset.
            Item {
                width: fnGroupX(KeyTable.fnRowGroups.length - 1)
                       + rowWidth(KeyTable.fnRowGroups[KeyTable.fnRowGroups.length - 1])
                height: Theme.capSize

                Repeater {
                    model: KeyTable.fnRowGroups
                    delegate: Row {
                        id: fnGroup
                        required property int index
                        required property var modelData
                        property var groupKeys: modelData
                        x: panel.fnGroupX(index)
                        spacing: Theme.capGap

                        Repeater {
                            model: fnGroup.groupKeys
                            delegate: KeyCap {
                                required property var modelData
                                keyName: modelData
                                onActivated: function (keyId) { panel.keyActivated(keyId, token) }
                            }
                        }
                    }
                }
            }

            // Tripled gap between the function row and the number row: the
            // column's own capGap plus this spacer.
            Item { width: 1; height: Theme.capGap * 2 }

            // -- main block | middle cluster | numpad --------------------------
            Row {
                id: mainBlock
                spacing: Theme.capClusterGap * 2

                Column {
                    spacing: Theme.capGap

                    Repeater {
                        model: KeyTable.mainRows
                        delegate: Row {
                            id: mainRow
                            required property var modelData
                            property var rowKeys: modelData
                            spacing: panel.mainRowSpacing(rowKeys)

                            Repeater {
                                model: mainRow.rowKeys
                                delegate: KeyCap {
                                    required property var modelData
                                    keyName: modelData
                                    onActivated: function (keyId) { panel.keyActivated(keyId, token) }
                                }
                            }
                        }
                    }
                }

                Column {
                    spacing: Theme.capGap

                    Repeater {
                        model: KeyTable.middleRows
                        delegate: Item {
                            id: midRow
                            required property var modelData
                            property var rowKeys: modelData
                            width: panel.middleClusterWidth
                            height: Theme.capSize

                            Row {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: Theme.capGap

                                Repeater {
                                    model: midRow.rowKeys
                                    delegate: KeyCap {
                                        required property var modelData
                                        keyName: modelData
                                        onActivated: function (keyId) { panel.keyActivated(keyId, token) }
                                    }
                                }
                            }
                        }
                    }
                }

                Row {
                    spacing: Theme.capGap

                    Column {
                        spacing: Theme.capGap

                        Repeater {
                            model: KeyTable.numpadMain
                            delegate: Row {
                                id: numRow
                                required property var modelData
                                property var rowKeys: modelData
                                spacing: Theme.capGap

                                Repeater {
                                    model: numRow.rowKeys
                                    delegate: KeyCap {
                                        required property var modelData
                                        keyName: modelData
                                        onActivated: function (keyId) { panel.keyActivated(keyId, token) }
                                    }
                                }
                            }
                        }
                    }

                    Column {
                        spacing: Theme.capGap

                        Repeater {
                            model: KeyTable.numpadRight
                            delegate: KeyCap {
                                required property var modelData
                                keyName: modelData
                                onActivated: function (keyId) { panel.keyActivated(keyId, token) }
                            }
                        }
                    }
                }
            }

            // Tripled gap between the keyboard's bottom row and the mouse,
            // with a thin separator bar marking the switch from keyboard to
            // mouse sections. Symmetric: 12px on each side of the bar.
            Item { width: 1; height: Theme.capGap * 2 }
            Rectangle {
                width: mainBlock.width
                height: Theme.borderWidth
                color: Theme.colorBorder
            }
            Item { width: 1; height: Theme.capGap * 2 }

            // -- mouse | controller ------------------------------------------
            // Both blocks below the keyboard, side by side, with the same thin
            // separator style between them as the horizontal bar above.
            Row {
                spacing: Theme.capGap

                Item { width: Theme.capGap * 2; height: 1 }

                // Mouse buttons and wheel: same caps and activation path as the
                // keyboard rows, arranged as a mouse rotated 90° CCW (front edge
                // = left). Cell geometry comes from KeyTable.mouseCells. The Row
                // only manages x, so center the shorter block vertically.
                Item {
                    width: KeyTable.mouseWidth
                    height: KeyTable.mouseHeight
                    y: (parent.height - KeyTable.mouseHeight) / 2

                    Repeater {
                        model: KeyTable.mouseCells
                        delegate: KeyCap {
                            required property var modelData
                            keyName: modelData.name
                            x: modelData.x
                            y: modelData.y
                            width: modelData.w
                            height: modelData.h
                            onActivated: function (keyId) { panel.keyActivated(keyId, token) }
                        }
                    }
                }

                Item { width: Theme.capGap * 2; height: 1 }

                Rectangle {
                    width: 1
                    height: KeyTable.controllerHeight
                    color: Theme.colorBorder
                }

                Item { width: Theme.capGap * 2; height: 1 }

                // Controller: Xbox 360 style, triggers-up, mirror-symmetric.
                // Cell geometry comes from KeyTable.controllerCells.
                Item {
                    width: KeyTable.controllerWidth
                    height: KeyTable.controllerHeight

                    Repeater {
                        model: KeyTable.controllerCells
                        delegate: KeyCap {
                            required property var modelData
                            keyName: modelData.name
                            x: modelData.x
                            y: modelData.y
                            width: modelData.w
                            height: modelData.h
                            onActivated: function (keyId) { panel.keyActivated(keyId, token) }
                        }
                    }
                }
            }
        }
    }

    // Ctrl+plus / Ctrl+minus / Ctrl+0. Both the main-row key (=) and the numpad key (+)
    // are accepted, on the numeric row and on the numeric keypad.
    Shortcut {
        sequences: ["Ctrl++", "Ctrl+="]
        onActivated: panel.zoomBy(panel.zoomStep)
    }
    Shortcut {
        sequences: ["Ctrl+-", "Ctrl+_"]
        onActivated: panel.zoomBy(-panel.zoomStep)
    }
    Shortcut {
        sequence: "Ctrl+0"
        onActivated: panel.resetZoom()
    }

    // Level read-out. Transient, so it never covers a cap for long and never competes
    // with the panel's own layout.
    Rectangle {
        id: zoomBadge
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: Theme.spacing
        anchors.bottomMargin: Theme.spacing + (hbar.visible ? hbar.height : 0)
        opacity: 0
        visible: opacity > 0
        radius: Theme.radius
        color: Theme.colorSurfaceRaised
        border.width: Theme.borderWidth
        border.color: Theme.colorBorderStrong
        implicitWidth: zoomBadgeText.implicitWidth + Theme.spacing * 2
        implicitHeight: zoomBadgeText.implicitHeight + Theme.spacingTight * 2

        Behavior on opacity { NumberAnimation { duration: 150 } }

        Text {
            id: zoomBadgeText
            anchors.centerIn: parent
            text: qsTr("Zoom %1%").arg(Math.round(panel.zoom * 100))
            color: Theme.colorText
            font.family: Theme.familyUi
            font.pointSize: Theme.sizeSmall
        }
    }

    Timer {
        id: zoomHint
        interval: 1200
        onTriggered: zoomBadge.opacity = 0
    }
}
