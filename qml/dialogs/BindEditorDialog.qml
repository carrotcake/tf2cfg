import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"

// Bind editor. Parity P30-P36.
Dialog {
    id: dialog

    /// Prefill for the "Existing Bind:" field; set from a key cap click.
    property string originalBind: ""

    /// Open the dialog for a TF2 key token: prefills bind "<token>".
    function openForKey(token) {
        dialog.originalBind = 'bind "' + token + '"'
        dialog.open()
    }

    title: "Dialog"          // parity P30: the widget's dialog title really is "Dialog"
    modal: true              // parity P36: widget used exec()
    width: 500
    height: 400
    padding: Theme.spacingWide
    font.family: Theme.familyUi
    font.pointSize: Theme.sizeUi
    standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    // Keep it modal like the widget's exec(): Escape rejects, clicking outside does not.
    closePolicy: Popup.CloseOnEscape

    onOpened: {
        existingBindField.text = dialog.originalBind
        // Center on the window each time it opens (a plain Dialog sits at 0,0).
        if (dialog.parent) {
            dialog.x = Math.max(0, Math.round((dialog.parent.width - dialog.width) / 2))
            dialog.y = Math.max(0, Math.round((dialog.parent.height - dialog.height) / 2))
        }
    }
    // Parity P35: the widget's accept slot was empty. Log instead of silently dropping.
    onAccepted: console.log("bind dialog accepted: " + existingBindField.text)

    background: Rectangle {
        color: Theme.colorSurface
        border.width: Theme.borderWidth
        border.color: Theme.colorBorderStrong
        radius: Theme.radius
    }

    // Grab bar: drag the dialog by its header. Movement is clamped to the application
    // window (the widget build's QDialog could be dragged anywhere on the desktop).
    // No fill: the dialog surface shows through, so its rounded top corners stay clean.
    header: Item {
        id: titleBar
        implicitHeight: 34

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: Theme.borderWidth
            color: Theme.colorBorder
        }

        Tf2Label {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacingWide
            anchors.rightMargin: Theme.spacingWide
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            text: dialog.title
            font.family: Theme.familyHeading
            font.pointSize: Theme.sizeHeading
        }

        MouseArea {
            id: dragArea
            anchors.fill: parent
            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor

            property point pressOffset: Qt.point(0, 0)

            onPressed: function (mouse) {
                dragArea.pressOffset = Qt.point(mouse.x, mouse.y)
            }
            onPositionChanged: function (mouse) {
                if (!dialog.parent)
                    return
                const maxX = Math.max(0, dialog.parent.width - dialog.width)
                const maxY = Math.max(0, dialog.parent.height - dialog.height)
                dialog.x = Math.max(0, Math.min(dialog.x + (mouse.x - dragArea.pressOffset.x), maxX))
                dialog.y = Math.max(0, Math.min(dialog.y + (mouse.y - dragArea.pressOffset.y), maxY))
            }
        }
    }

    ColumnLayout {
        spacing: Theme.spacing

        // -- existing bind (P31) ------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing

            Label {
                text: "Existing Bind:"
                color: Theme.colorText
                font.family: Theme.familyUi
                font.pointSize: Theme.sizeUi
                font.bold: true
            }

            Tf2TextField {
                id: existingBindField
                Layout.fillWidth: true
                placeholderText: "< not currently bound >"
            }
        }

        // -- the two tabs (P32) --------------------------------------------------
        Tf2TabBar {
            id: dialogTabs
            Layout.fillWidth: true
            model: ["Build-A-Bind", "Custom Command"]
            onActivated: function (index) { dialogTabs.currentIndex = index }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: dialogTabs.currentIndex

            // Parity P33: the widget's Build-A-Bind tab was empty - category and
            // command models are a future feature (docs/ui-qml-rewrite.md section 10).
            GridLayout {
                columns: 2
                columnSpacing: Theme.spacing
                rowSpacing: Theme.spacing

                Label {
                    text: "Category:"
                    color: Theme.colorText
                    font.family: Theme.familyUi
                    font.pointSize: Theme.sizeUi
                }
                Tf2ComboBox {
                    Layout.fillWidth: true
                    placeholderText: "< choose a category >"
                }

                Label {
                    text: "Command:"
                    color: Theme.colorText
                    font.family: Theme.familyUi
                    font.pointSize: Theme.sizeUi
                }
                Tf2ComboBox {
                    Layout.fillWidth: true
                    placeholderText: "< choose a command >"
                }

                Label {
                    text: "Arguments:"
                    color: Theme.colorText
                    font.family: Theme.familyUi
                    font.pointSize: Theme.sizeUi
                    Layout.alignment: Qt.AlignTop
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Theme.colorSurfaceSunken
                    border.width: Theme.borderWidth
                    border.color: Theme.colorBorder
                    radius: Theme.radius

                    Tf2Label {
                        anchors.centerIn: parent
                        text: "Nothing to enter yet."
                        color: Theme.colorTextMuted
                    }
                }
            }

            // Parity P34: plain text command editor, monospace, no wrapping.
            ScrollView {
                Layout.fillWidth: true

                TextArea {
                    id: customCommand
                    placeholderText: "< command goes here >"
                    font.family: Theme.familyMono
                    font.pointSize: Theme.sizeMono
                    color: Theme.colorText
                    placeholderTextColor: Theme.colorTextMuted
                    wrapMode: TextArea.NoWrap
                    selectByMouse: true
                    background: Rectangle {
                        color: Theme.colorSurfaceSunken
                        border.width: Theme.borderWidth
                        border.color: Theme.colorBorder
                        radius: Theme.radius
                    }
                }
            }
        }
    }
}
