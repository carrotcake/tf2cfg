import QtQuick
import QtQuick.Controls
import "../theme"

// The keyboard. Four clusters, same arrangement as the widget build
// (docs/ui-qml-rewrite.md section 8): function row, main block, middle cluster, numpad.
// Everything is data-driven from KeyTable.
Item {
    id: panel

    /// Emitted when a bindable cap is clicked.
    signal keyActivated(int keyId, string token)

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

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: Math.max(width, keyboard.width + Theme.spacingWide * 2)
        contentHeight: Math.max(height, keyboard.height + Theme.spacingWide * 2)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Column {
            id: keyboard
            x: (flick.contentWidth - width) / 2
            y: Theme.spacingWide
            spacing: Theme.capGap

            // -- function row --------------------------------------------------
            Row {
                spacing: Theme.capClusterGap

                Repeater {
                    model: KeyTable.fnRowGroups
                    delegate: Row {
                        id: fnGroup
                        property var groupKeys: modelData
                        spacing: Theme.capGap

                        Repeater {
                            model: fnGroup.groupKeys
                            delegate: KeyCap {
                                keyName: modelData
                                onActivated: function (keyId) { panel.keyActivated(keyId, token) }
                            }
                        }
                    }
                }
            }

            // -- main block | middle cluster | numpad --------------------------
            Row {
                spacing: Theme.capClusterGap * 2

                Column {
                    spacing: Theme.capGap

                    Repeater {
                        model: KeyTable.mainRows
                        delegate: Row {
                            id: mainRow
                            property var rowKeys: modelData
                            spacing: panel.mainRowSpacing(rowKeys)

                            Repeater {
                                model: mainRow.rowKeys
                                delegate: KeyCap {
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
                                property var rowKeys: modelData
                                spacing: Theme.capGap

                                Repeater {
                                    model: numRow.rowKeys
                                    delegate: KeyCap {
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
                                keyName: modelData
                                onActivated: function (keyId) { panel.keyActivated(keyId, token) }
                            }
                        }
                    }
                }
            }
        }
    }
}
