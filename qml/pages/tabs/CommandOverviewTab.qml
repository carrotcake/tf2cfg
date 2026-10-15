pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../components"
import "../../theme"

// Tab 0: every parsed CFG line, monospaced. Parity P15/P16/P38.
//
// The line stays the primary text; the columns to its right come from the command
// catalog (input signature, platform, context, cheat flag) - the "surface the newer
// catalog columns" item docs/legacy/allcmds-v5-migration.md section 5 left open.
Panel {
    id: tab

    title: "Commands"

    /// Context property hoisted to the root, the way ConfigEditorTab does it: nested
    /// delegates then read a local id instead of the context property.
    readonly property var cfgRef: cfg

    // Column widths, read by both the header strip and the row delegate so the two
    // cannot drift apart. Sized for the longest catalog value each column carries:
    // signatures like "[str] [flt] [flt] [flt]", "server commands", "windows", "cheat".
    readonly property int colInput: 140
    readonly property int colPlatform: 86
    readonly property int colContext: 130
    readonly property int colCheat: 56
    /// Everything but the line column, spacing included. Every column then carries an
    /// explicit width and the line column takes the remainder - a `Layout.fillWidth`
    /// line column lets the two rows negotiate different widths (the text's own width
    /// is the floor), and the columns stop lining up.
    readonly property int columnsWidth: colInput + colPlatform + colContext + colCheat
                                         + 4 * Theme.spacing

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: Theme.colorSurfaceSunken
        border.width: Theme.borderWidth
        border.color: Theme.colorBorder
        radius: Theme.radius
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingTight
            spacing: Theme.spacingTight

            // Catalog columns need names, or the row values are unreadable.
            RowLayout {
                id: headerRow
                Layout.fillWidth: true
                spacing: Theme.spacing

                Text {
                    // The list's width, not this row's: a layout width derived from its own
                    // children's preferred widths is circular and settles as 0.
                    Layout.preferredWidth: Math.max(0, commandList.width - tab.columnsWidth)
                    text: "line"
                    color: Theme.colorTextMuted
                    font.family: Theme.familyMono
                    font.pointSize: Theme.sizeSmall
                }
                Text {
                    Layout.preferredWidth: tab.colInput
                    text: "input"
                    color: Theme.colorTextMuted
                    font.family: Theme.familyMono
                    font.pointSize: Theme.sizeSmall
                }
                Text {
                    Layout.preferredWidth: tab.colPlatform
                    text: "platform"
                    color: Theme.colorTextMuted
                    font.family: Theme.familyMono
                    font.pointSize: Theme.sizeSmall
                }
                Text {
                    Layout.preferredWidth: tab.colContext
                    text: "context"
                    color: Theme.colorTextMuted
                    font.family: Theme.familyMono
                    font.pointSize: Theme.sizeSmall
                }
                Text {
                    Layout.preferredWidth: tab.colCheat
                    text: "cheat"
                    color: Theme.colorTextMuted
                    font.family: Theme.familyMono
                    font.pointSize: Theme.sizeSmall
                }
            }

            ListView {
                id: commandList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                reuseItems: true
                model: tab.cfgRef.commandModel
                ScrollBar.vertical: ScrollBar {
                    id: overviewScroll
                    policy: ScrollBar.AsNeeded
                    hoverEnabled: true
                    contentItem: Rectangle {
                        implicitWidth: Theme.spacingTight + 2
                        radius: implicitWidth / 2
                        color: overviewScroll.pressed ? Theme.colorAccent
                              : overviewScroll.hovered ? Theme.colorTextMuted
                              : Theme.colorBorderStrong
                    }
                }

                delegate: RowLayout {
                    id: rowDelegate

                    // Role "line" is args.join(" "), exactly what the widget list showed.
                    // No id here: an id named "line" would shadow this property.
                    required property string line
                    required property string command

                    // The catalog record behind the line's head command - the same
                    // lookup the Config Editor's guided pane does. Not every line has
                    // one (session rows, unknown commands), and the columns stay blank
                    // there rather than hiding the line.
                    readonly property var rec: tab.cfgRef.consoleModel.findCommand(command)
                    readonly property bool known: rec !== undefined && rec.name !== undefined

                    width: commandList.width
                    spacing: Theme.spacing

                    Text {
                        id: lineText
                        Layout.preferredWidth: Math.max(0, rowDelegate.width - tab.columnsWidth)
                        text: rowDelegate.line
                        color: Theme.colorText
                        font.family: Theme.familyMono
                        font.pointSize: Theme.sizeMono
                        elide: Text.ElideRight
                    }
                    Text {
                        id: inputText
                        // Field 8: "int", "flt", "<int>", "bind [str] [str]", "none".
                        Layout.preferredWidth: tab.colInput
                        text: rowDelegate.known ? rowDelegate.rec.input : ""
                        color: Theme.colorTextMuted
                        font.family: Theme.familyMono
                        font.pointSize: Theme.sizeSmall
                        elide: Text.ElideRight
                    }
                    Text {
                        // Field 9: blank when universal (almost every row), so only the
                        // platform-specific entries stand out.
                        Layout.preferredWidth: tab.colPlatform
                        text: rowDelegate.known && rowDelegate.rec.platform !== "universal"
                              ? rowDelegate.rec.platform : ""
                        color: Theme.colorTextMuted
                        font.family: Theme.familyMono
                        font.pointSize: Theme.sizeSmall
                        elide: Text.ElideRight
                    }
                    Text {
                        // Field 10: one primary usage context.
                        Layout.preferredWidth: tab.colContext
                        text: rowDelegate.known ? rowDelegate.rec.context : ""
                        color: Theme.colorTextMuted
                        font.family: Theme.familyMono
                        font.pointSize: Theme.sizeSmall
                        elide: Text.ElideRight
                    }
                    Text {
                        // Field 6 tags: only "cheat" is actionable while reading a config.
                        Layout.preferredWidth: tab.colCheat
                        text: rowDelegate.known && rowDelegate.rec.cheat ? "cheat" : ""
                        color: Theme.colorRed
                        font.family: Theme.familyMono
                        font.pointSize: Theme.sizeSmall
                        elide: Text.ElideRight
                    }

                    // Parity P16: the widget list showed a pointing-hand cursor. A handler,
                    // not a MouseArea - the rows are not clickable, and a MouseArea here
                    // would swallow every press for nothing.
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                }
            }
        }

        Tf2Label {
            anchors.centerIn: parent
            visible: commandList.count === 0
            text: "No config loaded."
            color: Theme.colorTextMuted
        }
    }
}
