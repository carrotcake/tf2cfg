pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "../../components"
import "../../theme"

// Custom Build-A-Bind panel for the spy disguise command (docs/DESIGN-enum-panels.md
// §7, verified in-game 2026-10-13). Same contract as VoiceMenuPanel:
//   - property var initialArgs / signal authored(string text) / function prefill(parts)
// Emission is always the two-argument form `disguise <class> <team>` (class first).
// Argument ids, per the in-game check: class 1 scout, 2 sniper, 3 soldier, 4 demoman,
// 5 medic, 6 heavy, 7 pyro, 8 spy, 9 engineer; team -2 friendly, -1 enemy (auto-picks
// the opposing team), 1 blu, 2 red.
Item {
    id: panel

    implicitHeight: contentCol.implicitHeight

    property var initialArgs: []
    property int classId: 1
    property int teamId: -1

    signal authored(string text)

    readonly property var classes: [
        { id: 1, label: "Scout" },
        { id: 2, label: "Sniper" },
        { id: 3, label: "Soldier" },
        { id: 4, label: "Demoman" },
        { id: 5, label: "Medic" },
        { id: 6, label: "Heavy" },
        { id: 7, label: "Pyro" },
        { id: 8, label: "Spy" },
        { id: 9, label: "Engineer" }
    ]
    readonly property var teams: [
        { id: -2, label: "Friendly" },
        { id: -1, label: "Enemy" },
        { id: 1, label: "BLU" },
        { id: 2, label: "RED" }
    ]

    onInitialArgsChanged: {
        if (initialArgs.length === 0) {
            classId = 1
            teamId = -1
            emitAuthored()
            return
        }
        adopt(initialArgs)
    }

    function authoredText() {
        return "disguise " + classId + " " + teamId
    }

    function emitAuthored() {
        authored(authoredText())
    }

    /// Adopt parsed arguments. Returns false (state untouched, nothing emitted) for
    /// anything the panel does not model: non-integers or an unknown id.
    function adopt(args) {
        const cls = args.length >= 1 ? Number(args[0]) : NaN
        const team = args.length >= 2 ? Number(args[1]) : NaN
        if (!Number.isInteger(cls) || !classes.some(c => c.id === cls)
                || !Number.isInteger(team) || !teams.some(t => t.id === team))
            return false
        classId = cls
        teamId = team
        emitAuthored()
        return true
    }

    function prefill(parts) {
        adopt(parts.slice(1))
    }

    function selectClass(id) {
        classId = id
        emitAuthored()
    }

    function selectTeam(id) {
        teamId = id
        emitAuthored()
    }

    ColumnLayout {
        id: contentCol
        width: parent.width
        spacing: Theme.spacingTight

        Tf2Label {
            Layout.fillWidth: true
            text: "Enemy/Friendly pick the disguise's team relative to yours; the two "
                  + "explicit colors pin it regardless of situation."
            color: Theme.colorTextMuted
            font.pointSize: Theme.sizeSmall
            wrapMode: Text.WordWrap
        }

        Flow {
            Layout.fillWidth: true
            spacing: Theme.spacingTight

            Repeater {
                model: panel.classes

                delegate: Tf2Button {
                    required property var modelData

                    text: modelData.label
                    checkable: true
                    flat: !checked
                    checked: panel.classId === modelData.id
                    onClicked: panel.selectClass(modelData.id)
                }
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: Theme.spacingTight

            Repeater {
                model: panel.teams

                delegate: Tf2Button {
                    required property var modelData

                    text: modelData.label
                    checkable: true
                    flat: !checked
                    checked: panel.teamId === modelData.id
                    onClicked: panel.selectTeam(modelData.id)
                }
            }
        }
    }
}
