pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../components"
import "../../theme"

// Generic Build-A-Bind panel for commands whose single argument is a small named
// enumeration (docs/DESIGN-enum-panels.md). Same contract as VoiceMenuPanel
// (docs/DESIGN-custom-panels.md §3) - initialArgs / authored(text) / prefill(parts) -
// plus data the dialog injects from the registry entry:
//   commandName      - the selected command (one component serves build + destroy)
//   valueType        - "int" or "str"; only affects prefill matching
//   options          - [{ value, label }], in authored order
//   compoundTemplate - optional; "destroy {v}; build {v}" enables the Rebuild box,
//                      with {v} replaced by the selected value. {v}'s preceding word
//                      is the partner command name the compound prefill looks for.
// Same safety rule: an unknown bound value keeps the panel state and emits nothing,
// so a foreign command variant is never silently rewritten.
Item {
    id: panel

    implicitHeight: contentCol.implicitHeight

    property var initialArgs: []
    property string commandName: ""
    property string valueType: "str"
    property var options: []
    property string compoundTemplate: ""
    property bool rebuild: false
    property int selectedIndex: -1

    signal authored(string text)

    onInitialArgsChanged: {
        if (initialArgs.length === 0) {
            reset()
            return
        }
        applyTokens(normalizeTokens(initialArgs))
    }

    /// Strip trailing semicolons: compound binds arrive as tokens like "2;".
    function normalizeTokens(args) {
        return args.map(function (t) { return String(t).replace(/;$/, "") })
    }

    /// Registry options may carry numeric or string values; match a bound token
    /// accordingly (int numerically, str case-insensitively).
    function matchIndex(token) {
        for (let i = 0; i < options.length; ++i) {
            const v = options[i].value
            if (valueType === "int" ? Number(token) === v
                                    : String(token).toLowerCase() === String(v).toLowerCase())
                return i
        }
        return -1
    }

    /// The partner command name is the word in front of {v} in the template.
    function partnerName() {
        const head = compoundTemplate.split("{v}")[0].trim()
        const words = head.split(/\s+/)
        return words[words.length - 1]
    }

    function authoredText() {
        if (selectedIndex < 0)
            return ""
        const value = String(options[selectedIndex].value)
        return rebuild && compoundTemplate !== ""
            ? compoundTemplate.replace("{v}", value)
            : commandName + " " + value
    }

    function emitAuthored() {
        authored(authoredText())
    }

    /// Default state: first option, no compound. Emits, so the Custom Command field
    /// never keeps a stale command from the previously selected command.
    function reset() {
        selectedIndex = options.length > 0 ? 0 : -1
        rebuild = false
        emitAuthored()
    }

    /// Adopt parsed arguments (without the command name). Unknown values keep the
    /// state and emit nothing.
    function applyTokens(tokens) {
        // Compound: "value partner value" after the command name, e.g. the bind
        // "destroy 2; build 2" reached the build/destroy panel as ["2","build","2"].
        if (tokens.length === 3 && compoundTemplate !== "") {
            const a = matchIndex(tokens[0])
            const partner = tokens[1].toLowerCase()
            const b = matchIndex(tokens[2])
            if (a >= 0 && b >= 0 && a === b
                    && partner !== commandName.toLowerCase()
                    && partner === partnerName().toLowerCase()) {
                selectedIndex = a
                rebuild = true
                emitAuthored()
                return
            }
        }
        if (tokens.length >= 1) {
            const idx = matchIndex(tokens[0])
            if (idx >= 0) {
                selectedIndex = idx
                rebuild = false
                emitAuthored()
                return
            }
        }
    }

    function prefill(parts) {
        applyTokens(normalizeTokens(parts.slice(1)))
    }

    function selectOption(index) {
        selectedIndex = index
        emitAuthored()
    }

    ColumnLayout {
        id: contentCol
        width: parent.width
        spacing: Theme.spacingTight

        Flow {
            Layout.fillWidth: true
            spacing: Theme.spacingTight

            Repeater {
                model: panel.options

                delegate: Tf2Button {
                    required property var modelData
                    required property int index

                    text: modelData.label
                    checkable: true
                    flat: !checked
                    checked: panel.selectedIndex === index
                    onClicked: panel.selectOption(index)
                }
            }
        }

        CheckBox {
            id: rebuildCheck

            visible: panel.compoundTemplate !== ""
            text: "Rebuild: " + panel.compoundTemplate.replace("{v}", panel.selectedIndex >= 0
                                                                     ? String(panel.options[panel.selectedIndex].value) : "?")
            checked: panel.rebuild
            onToggled: {
                panel.rebuild = rebuildCheck.checked
                panel.emitAuthored()
            }
        }
    }
}
