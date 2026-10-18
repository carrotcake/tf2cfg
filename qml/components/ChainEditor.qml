pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

// A ';'-joined command chain, one text field per segment, with add/remove/reorder
// (docs/DESIGN-config-editor.md §5). Shared by the Config Editor (compound lines and
// alias bodies) and BindEditorDialog's "Alias" tab. The segments are raw text - the
// caller quotes/unquotes and joins; this component only edits the piece list.
ColumnLayout {
    id: root

    /// Emitted after any segment edit, so the caller can regenerate its line.
    signal changed()

    /// Fill from a list of raw segment strings (replaces the current content).
    function setSegments(list) {
        segModel.clear()
        for (let i = 0; i < list.length; ++i)
            segModel.append({ text: String(list[i]) })
    }

    /// The non-empty trimmed segments, in display order.
    function segmentList() {
        const out = []
        for (let i = 0; i < segModel.count; ++i) {
            const text = segModel.get(i).text.trim()
            if (text !== "")
                out.push(text)
        }
        return out
    }

    function moveSegment(index, delta) {
        const target = index + delta
        if (target < 0 || target >= segModel.count)
            return
        segModel.move(index, target, 1)
        root.changed()
    }

    function removeSegment(index) {
        segModel.remove(index)
        root.changed()
    }

    spacing: Theme.spacingTight

    Repeater {
        model: ListModel { id: segModel }

        delegate: RowLayout {
            id: segmentRow

            required property int index
            required property var model

            Layout.fillWidth: true
            spacing: Theme.spacingTight

            TextField {
                id: segmentField
                Layout.fillWidth: true
                text: segmentRow.model.text
                color: Theme.colorText
                font.family: Theme.familyMono
                font.pointSize: Theme.sizeMono
                hoverEnabled: true
                selectByMouse: true
                background: Rectangle {
                    color: Theme.colorSurface
                    radius: Theme.radius
                    border.width: Theme.borderWidth
                    border.color: segmentField.activeFocus ? Theme.colorAccent
                                                           : Theme.colorBorder
                }
                onTextEdited: {
                    segmentRow.model.text = text
                    root.changed()
                }
                Accessible.name: "Chain segment " + (segmentRow.index + 1)
                Accessible.role: Accessible.EditableText
            }

            Tf2Button {
                text: "\u2191"
                flat: true
                enabled: segmentRow.index > 0
                onClicked: root.moveSegment(segmentRow.index, -1)
                Accessible.name: "Move segment up"
            }

            Tf2Button {
                text: "\u2193"
                flat: true
                enabled: segmentRow.index < segModel.count - 1
                onClicked: root.moveSegment(segmentRow.index, 1)
                Accessible.name: "Move segment down"
            }

            Tf2Button {
                text: "\u2715"
                flat: true
                onClicked: root.removeSegment(segmentRow.index)
                Accessible.name: "Remove segment"
            }
        }
    }

    Tf2Button {
        text: "+ add segment"
        flat: true
        onClicked: {
            segModel.append({ text: "" })
            root.changed()
        }
    }
}