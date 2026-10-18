pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "../../components"
import "../../theme"

// Custom Build-A-Bind panel for TF2's voicemenu quick-chat command
// (docs/DESIGN-custom-panels.md). Contract every custom panel fulfils:
//   - property var initialArgs - arguments of the binding being edited, without the
//     command name; [] when the key carries no voicemenu binding. An empty list
//     resets the panel to its default ("open menu 0") and emits it.
//   - signal authored(string text) - the panel's authored command; the dialog
//     mirrors it into the Custom Command field, which is what OK applies.
//   - function prefill(parts) - adopt a parsed binding; parts[0] is the command
//     name itself. Malformed input keeps the current state and emits nothing, so a
//     foreign voicemenu variant is never silently rewritten.
Item {
    id: panel

    implicitHeight: contentCol.implicitHeight

    property var initialArgs: []
    property int menuIndex: 0
    // -1 = the one-argument "open this menu" form.
    property int itemIndex: -1

    signal authored(string text)

    // Generic line labels (wiki community mapping). Menus 0/1 have 9 lines, menu 2
    // has 8; class-specific variants are intentionally not modeled - the value a
    // key carries is identical for every class.
    readonly property var menus: [
        {
            key: "Z",
            title: "Voice",
            lines: ["Medic!", "Thanks!", "Go go go!", "Move up!", "Go left!", "Go right!",
                    "Yes!", "No!", "Pass to me!"]
        },
        {
            key: "X",
            title: "Alerts",
            lines: ["Incoming!", "Spy!", "Sentry ahead!", "Teleporter here!",
                    "Dispenser here!", "Sentry here!", "Activate charge!",
                    "\u00dcberCharge ready!", "Pass to me!"]
        },
        {
            key: "C",
            title: "Reactions",
            lines: ["Help!", "Battle cry!", "Cheers!", "Jeers!", "Positive!", "Negative!",
                    "Nice shot!", "Good job!"]
        }
    ]

    onInitialArgsChanged: {
        if (initialArgs.length === 0) {
            menuIndex = 0
            itemIndex = -1
            emitAuthored()
            return
        }
        adopt(initialArgs)
    }

    function authoredText() {
        return itemIndex >= 0 ? "voicemenu " + menuIndex + " " + itemIndex
                              : "voicemenu " + menuIndex
    }

    function emitAuthored() {
        authored(authoredText())
    }

    /// Adopt parsed arguments. Returns false (state untouched, nothing emitted) for
    /// anything the panel does not model: non-integers, an unknown menu, or an item
    /// beyond the menu's own line count.
    function adopt(args) {
        const menu = args.length >= 1 ? Number(args[0]) : NaN
        const item = args.length >= 2 ? Number(args[1]) : NaN
        if (!Number.isInteger(menu) || menu < 0 || menu >= menus.length)
            return false
        const count = menus[menu].lines.length
        const validItem = Number.isInteger(item) && item >= 0 && item < count
        if (args.length >= 2 && !validItem)
            return false
        menuIndex = menu
        itemIndex = validItem ? item : -1
        emitAuthored()
        return true
    }

    function prefill(parts) {
        adopt(parts.slice(1))
    }

    function selectMenu(index) {
        if (menuIndex === index)
            return
        menuIndex = index
        itemIndex = -1 // the old menu's line no longer applies
        emitAuthored()
    }

    function selectLine(index) {
        itemIndex = index
        emitAuthored()
    }

    ColumnLayout {
        id: contentCol
        width: parent.width
        spacing: Theme.spacingTight

        Tf2Label {
            Layout.fillWidth: true
            text: "One argument opens the quick-chat menu in-game; picking a line makes "
                  + "your character say it immediately."
            color: Theme.colorTextMuted
            font.pointSize: Theme.sizeSmall
            wrapMode: Text.WordWrap
        }

        RowLayout {
            spacing: Theme.spacingTight

            Repeater {
                model: panel.menus

                delegate: Tf2Button {
                    required property var modelData
                    required property int index

                    text: modelData.key + " - " + modelData.title
                    checkable: true
                    flat: !checked
                    checked: panel.menuIndex === index
                    onClicked: panel.selectMenu(index)
                }
            }
        }

        Tf2Button {
            text: "Open this menu"
            flat: panel.itemIndex !== -1
            onClicked: {
                panel.itemIndex = -1
                panel.emitAuthored()
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: Theme.spacingTight

            Repeater {
                model: panel.menus[panel.menuIndex].lines

                delegate: Tf2Button {
                    required property var modelData
                    required property int index

                    text: index + ": " + modelData
                    checkable: true
                    flat: !checked
                    checked: panel.itemIndex === index
                    onClicked: panel.selectLine(index)
                }
            }
        }
    }
}
