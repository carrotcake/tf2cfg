import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

// Bordered surface with an optional centered heading. Used for the banner and the two
// start-screen panels (docs/ui-qml-rewrite.md section 8).
Rectangle {
    id: panel

    property string title: ""
    /// Content goes into a ColumnLayout, so Layout.* attached properties work.
    default property alias body: contentLayout.data

    color: Theme.colorSurface
    border.width: Theme.borderWidth
    border.color: Theme.colorBorder
    radius: Theme.radius

    ColumnLayout {
        id: root
        anchors.fill: parent
        anchors.margins: Theme.spacingWide
        spacing: Theme.spacing

        Label {
            id: titleLabel
            visible: panel.title !== ""
            text: panel.title
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.family: Theme.familyHeading
            font.pointSize: Theme.sizeHeading
            font.bold: true
            color: Theme.colorText
        }

        ColumnLayout {
            id: contentLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacing
        }
    }
}
