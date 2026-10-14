import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"
import "panels"
pragma ComponentBehavior: Bound

// Bind editor. Parity P30-P36.
Dialog {
    id: dialog

    /// Context property handle for qualified access (avoids repeated advisories).
    readonly property var cfgRef: cfg
    objectName: "bindEditorDialog"

    /// Prefill for the "Existing Bind:" field; set from a key cap click.
    property string originalBind: ""

    /// TF2 key token this dialog is editing (e.g. "space", "kp_home").
    property string keyToken: ""

    /// The command the loaded config has on this key, or "" when it is unbound.
    readonly property string boundCommand: {
        if (dialog.keyToken === "")
            return ""
        const command = cfg.binds[dialog.keyToken.toLowerCase()]
        return command === undefined ? "" : command
    }

    /// Record of the command picked in the Build-A-Bind tab, from
    /// cfg.consoleModel.commandAt(). null until a command is selected.
    property var commandInfo: null

    /// Live suggestion entries for the typed command text (whole-catalog
    /// contains-matches from cfg.consoleModel.suggest()).
    property var suggestionList: []

    /// Highlighted entry in the suggestion pop-down (-1 = none).
    property int suggestionIndex: -1

    /// True when user is navigating with arrow keys or mouse wheel.
    /// When true, show all entries with scroll bar; when false, show only first 8.
    property bool browsing: false

    /// Toggle mode needs the engine's incrementvar command to know where to stop:
    /// bool cvars are always 0..1, int/flt need the CSV min/max.
    readonly property bool toggleAvailable:
        commandInfo !== null && commandInfo.isCvar
        && (commandInfo.input === "bool"
            || (commandInfo.min !== "" && commandInfo.max !== ""))

    // Custom command panels (docs/DESIGN-custom-panels.md, docs/DESIGN-enum-panels.md):
    // commands whose Build-A-Bind authoring is particular get a hand-written QML panel
    // that replaces the generic argument editors. Registry key = normalized command
    // name; the value carries the panel Component plus the data fields the panel type
    // consumes (see the panel files' headers). Adding a panel is one
    // qml/dialogs/panels file, one qrc entry and one entry here.
    readonly property string activePanelName:
        commandInfo !== null && customPanels.hasOwnProperty(commandInfo.name.toLowerCase())
        ? commandInfo.name.toLowerCase() : ""
    readonly property bool customPanelActive: activePanelName !== ""

    property var classOptions: [
        { value: "scout", label: "Scout" },
        { value: "soldier", label: "Soldier" },
        { value: "pyro", label: "Pyro" },
        { value: "demoman", label: "Demoman" },
        { value: "heavyweapons", label: "Heavy" },
        { value: "engineer", label: "Engineer" },
        { value: "medic", label: "Medic" },
        { value: "sniper", label: "Sniper" },
        { value: "spy", label: "Spy" },
        { value: "random", label: "Random" },
        { value: "auto", label: "Auto" }
    ]
    property var buildingOptions: [
        { value: "0 0", label: "Dispenser" },
        { value: "1 0", label: "Teleporter entrance" },
        { value: "1 1", label: "Teleporter exit" },
        { value: "2 0", label: "Sentry gun" }
    ]

    property var customPanels: ({
        "voicemenu": { component: voicemenuPanel },
        "jointeam": { component: enumPanel, valueType: "str", options: [
            { value: "red", label: "RED" },
            { value: "blu", label: "BLU" },
            { value: "auto", label: "Auto-assign" },
            { value: "spectate", label: "Spectate" }
        ] },
        "joinclass": { component: enumPanel, valueType: "str", options: classOptions },
        "join_class": { component: enumPanel, valueType: "str", options: classOptions },
        "build": { component: enumPanel, valueType: "str", options: buildingOptions,
                   compoundTemplate: "destroy {v}; build {v}" },
        "destroy": { component: enumPanel, valueType: "str", options: buildingOptions,
                     compoundTemplate: "destroy {v}; build {v}" },
        "eureka_teleport": { component: enumPanel, valueType: "int", options: [
            { value: 0, label: "Teleport home" },
            { value: 1, label: "Teleporter exit" }
        ] },
        "coach_command": { component: enumPanel, valueType: "int", options: [
            { value: 0, label: "Attack" },
            { value: 1, label: "Defend" },
            { value: 2, label: "Look" },
            { value: 3, label: "Go" }
        ] },
        "next_map_vote": { component: enumPanel, valueType: "int", options: [
            { value: 0, label: "Option 1" },
            { value: 1, label: "Option 2" },
            { value: 2, label: "Option 3" }
        ] },
        "loot_response": { component: enumPanel, valueType: "str", options: [
            { value: "common", label: "Common" },
            { value: "rare", label: "Rare" },
            { value: "ultra_rare", label: "Ultra rare" }
        ] },
        "load_itempreset": { component: enumPanel, valueType: "int", options: [
            { value: 0, label: "Preset A" },
            { value: 1, label: "Preset B" },
            { value: 2, label: "Preset C" },
            { value: 3, label: "Preset D" }
        ] },
        "disguise": { component: disguisePanel }
    })

    Component {
        id: voicemenuPanel
        VoiceMenuPanel {}
    }

    Component {
        id: disguisePanel
        DisguisePanel {}
    }

    Component {
        id: enumPanel
        EnumChoicePanel {}
    }

    /// The key's bound command split into tokens (empty when unbound), for a custom
    /// panel's initial state.
    readonly property var boundParts: boundCommand === "" ? [] : boundCommand.trim().split(/\s+/)

    /// "[str] <flt> bool none" -> editor specs. [] = mandatory, <> = optional,
    /// bare tokens are cvar values; "none" means no arguments at all.
    function parseSignature(sig) {
        if (!sig || sig === "none")
            return []
        return sig.split(/\s+/).filter(function (t) { return t !== "" }).map(function (t) {
            let m = t.match(/^\[([a-z]+)\]$/)
            if (m)
                return { kind: m[1], optional: false }
            m = t.match(/^<([a-z]+)>$/)
            if (m)
                return { kind: m[1], optional: true }
            return { kind: t, optional: false }
        })
    }

    // Radix 10 on purpose: without it "0x10" parses as 16.
    function validBottom(bound) { return bound === "" ? -2147483647 : parseInt(bound, 10) }
    function validTop(bound) { return bound === "" ? 2147483647 : parseInt(bound, 10) }

    function splitArgs(text) {
        const parts = []
        let cur = ""
        let inQuotes = false
        for (let i = 0; i < text.length; i++) {
            const c = text.charAt(i)
            if (inQuotes) {
                if (c === '"') {
                    if (i + 1 < text.length && text.charAt(i + 1) === '"') {
                        cur += '"'
                        i++
                    } else
                        inQuotes = false
                } else
                    cur += c
            } else if (c === '"') {
                inQuotes = true
            } else if (/\s/.test(c)) {
                if (cur !== "")
                    parts.push(cur)
                cur = ""
            } else
                cur += c
        }
        if (cur !== "")
            parts.push(cur)
        return parts
    }

    /// Old one-argument build/destroy binds (community mapping, exit = 3) migrate to
    /// the verified two-argument convention when the dialog opens: the field shows
    /// the modernized form, OK commits it, Cancel keeps the bound command untouched
    /// (docs/DESIGN-enum-panels.md §7). Quote-aware: a semicolon or a "build N" inside
    /// a quoted string is never rewritten.
    function modernizedCommand(text) {
        const map = { "0": "0 0", "1": "1 0", "2": "2 0", "3": "1 1" }
        let out = ""
        let plain = ""
        let inQuotes = false
        const flush = function () {
            out += plain.replace(/(build|destroy)\s+([0-3])(?=\s*(;|$))/gi,
                                 function (_m, verb, n) {
                                     return verb.toLowerCase() + " " + map[n]
                                 })
            plain = ""
        }
        for (let i = 0; i < text.length; i++) {
            const c = text.charAt(i)
            if (inQuotes) {
                if (c === '"') {
                    if (i + 1 < text.length && text.charAt(i + 1) === '"') {
                        out += '""'
                        i++
                    } else {
                        inQuotes = false
                        out += '"'
                        flush()
                    }
                } else
                    out += c
            } else if (c === '"') {
                flush()
                inQuotes = true
                out += '"'
            } else
                plain += c
        }
        flush()
        return out
    }

    function prefillFromCommand() {
        const text = customCommand.text.trim()
        if (text === "")
            return
        const parts = splitArgs(text)
        if (parts.length === 0)
            return
        const name = parts[0]
        const rec = dialog.cfgRef.consoleModel.findCommand(name)
        if (rec.name === undefined)
            return

        // Match settleCommand's filter handling so a hidden command becomes visible.
        if (rec.context !== undefined)
            applyCategoryFilter(rec.context)
        if (rec.platform !== undefined)
            applyPlatformFilter(rec.platform)

        const idx = commandCombo.find(name, Qt.MatchFixedString)
        if (idx < 0)
            return
        commandCombo.currentIndex = idx
        commandCombo.editText = name
        rebuildArgs()
        if (!commandInfo)
            return

        // A custom panel owns authoring for this command; hand it the parsed parts
        // and skip the generic arg-model filling entirely.
        if (dialog.customPanelActive) {
            if (customPanelLoader.item)
                customPanelLoader.item.prefill(parts)
            return
        }

        if (commandInfo.name.toLowerCase() === "incrementvar") {
            modeToggle.checked = true
            const values = parts.slice(1)
            for (let i = 0; i < argModel.count && i < values.length; i++)
                argModel.get(i).value = values[i]
            for (let i = 0; i < argModel.count; i++)
                if (i < values.length)
                    argModel.get(i).included = true
            return
        }

        // Bool cvars appear as their bare name when toggling; otherwise the first
        // extra token (if any) is treated as a fixed value.
        if (commandInfo.isCvar) {
            if (parts.length <= 1)
                modeToggle.checked = commandInfo.input === "bool"
            else {
                modeToggle.checked = false
                if (argModel.count > 0)
                    argModel.get(0).value = parts[1]
            }
            return
        }

        for (let i = 0; i < argModel.count; i++) {
            if (parts[i + 1] !== undefined) {
                argModel.get(i).value = parts[i + 1]
                argModel.get(i).included = true
            } else if (argModel.get(i).optional)
                argModel.get(i).included = false
        }
    }

    /// Hand the custom panel (if any) the registry data and the key's bound arguments
    /// as its initial state. Inline components load synchronously, so item is ready
    /// on this call. Panels that do not declare a field skip it (`voicemenu` is
    /// bespoke and carries its own data).
    function updateCustomPanel() {
        if (!dialog.customPanelActive || !customPanelLoader.item)
            return
        const entry = dialog.customPanels[dialog.activePanelName]
        const item = customPanelLoader.item
        if ('commandName' in item)
            item.commandName = dialog.activePanelName
        if ('valueType' in item)
            item.valueType = entry.valueType !== undefined ? entry.valueType : "str"
        if ('options' in item)
            item.options = entry.options !== undefined ? entry.options : []
        if ('compoundTemplate' in item)
            item.compoundTemplate = entry.compoundTemplate !== undefined
                                    ? entry.compoundTemplate : ""
        const name = dialog.activePanelName
        const bound = dialog.boundParts
        item.initialArgs =
            bound.length > 0 && bound[0].toLowerCase() === name ? bound.slice(1) : []
    }

    /// Rebuild the argument editors from the selected command's signature.
    function rebuildArgs() {
        commandInfo = commandCombo.currentIndex >= 0
                ? cfg.consoleModel.commandAt(commandCombo.currentIndex) : null
        updateCustomPanel()
        argModel.clear()
        if (!commandInfo)
            return
        const specs = parseSignature(commandInfo.input)
        // Per-argument authoring bounds from the catalog (docs/DESIGN-arg-bounds.md):
        // one entry per signature token, bounds only on the sides the companion file
        // verified. Cvars have no argsSpec and keep their row range on every editor,
        // as before.
        const argSpecs = commandInfo.argsSpec || []
        specs.forEach(function (spec, i) {
            const bs = argSpecs[i] && argSpecs[i].index === i ? argSpecs[i] : null
            const kind = bs ? (bs.isFloat ? "flt" : spec.kind) : spec.kind
            argModel.append({
                kind: kind,
                optional: spec.optional,
                included: !spec.optional,
                label: "param " + (i + 1) + " (" + kind + ")",
                value: specs.length === 1 && commandInfo.def !== "" ? commandInfo.def
                                                                    : defaultValueFor(kind),
                min: bs ? (bs.hasMin ? bs.min : "") : commandInfo.min,
                max: bs ? (bs.hasMax ? bs.max : "") : commandInfo.max
            })
        })
        regen()
        if (commandInfo && commandInfo.isCvar && commandInfo.input === "bool")
            bindToggleCheck.checked = modeToggle.checked
    }

    function defaultValueFor(kind) {
        if (kind === "int")
            return "0"
        if (kind === "flt")
            return "0.0"
        return ""
    }

    /// Fall back to the current selection when the field holds text that is not
    /// a catalog command (so a non-existent command is never carried forward).
    function revertEdit() {
        commandCombo.editText = commandCombo.currentIndex >= 0 ? commandCombo.currentText : ""
    }

    /// The typed portion of the command field: the engine's inline auto-complete
    /// appends the completed remainder as a selected suffix, which must not count
    /// as typed text.
    function typedPrefix() {
        const edit = commandCombo.contentItem
        const text = commandCombo.editText
        if (edit.selectionStart !== edit.selectionEnd
                && Math.max(edit.selectionStart, edit.selectionEnd) === text.length)
            return text.substring(0, Math.min(edit.selectionStart, edit.selectionEnd))
        return text
    }

    /// Rebuild + show/hide the suggestion pop-down for the typed text.
    function updateSuggestions() {
        suggestionIndex = -1
        browsing = false  // typing resets browsing mode
        const typed = typedPrefix()
        if (typed === "" || !commandCombo.activeFocus
                || (commandCombo.currentIndex >= 0 && typed === commandCombo.currentText)) {
            suggestionPopup.close()
            return
        }
        commandCombo.popup.close()
        suggestionList = cfg.consoleModel.suggest(typed)
        if (suggestionList.length > 0)
            suggestionPopup.open()
        else
            suggestionPopup.close()
    }

    /// Move the suggestion highlight (delta = +1/-1). Consumed when handled.
    /// Browsing the real dropdown keeps the combo's default Up/Down behavior.
    function moveSuggestion(delta) {
        if (commandCombo.popup.opened)
            return false
        if (!suggestionPopup.opened) {
            updateSuggestions()
            if (suggestionPopup.opened) {
                suggestionIndex = delta > 0 ? 0 : suggestionList.length - 1
                browsing = true  // arrow key navigation enters browsing mode
                return true
            }
            // nothing to suggest: an empty field opens the browse list, typed
            // text without matches does not jump the selection
            if (typedPrefix() === "")
                commandCombo.popup.open()
            return true
        }
        const next = suggestionIndex + delta
        if (next >= 0 && next < suggestionList.length) {
            suggestionIndex = next
            browsing = true  // arrow key navigation enters browsing mode
        }
        return true
    }

    function commitSuggestion(name) {
        commandCombo.editText = name
        settleCommand()
    }

    /// Point the filter dropdowns (and the model filters behind them) at one of
    /// their options; "all" clears the corresponding model filter.
    function applyCategoryFilter(context) {
        cfg.consoleModel.contextFilter = context === "all" ? "" : context
        categoryCombo.currentIndex = categoryCombo.find(context, Qt.MatchFixedString)
    }

    function applyPlatformFilter(platform) {
        cfg.consoleModel.platformFilter = platform === "all" ? "" : platform
        platformCombo.currentIndex = platformCombo.find(platform, Qt.MatchFixedString)
    }

    /// Settle the command field: commit the typed name when it exists in allcmds,
    /// else revert. Runs on Enter (accepted) and on focus loss, matching the
    /// editable-combo behavior in widgets.
    function settleCommand() {
        const typed = commandCombo.editText.trim()
        const rec = cfg.consoleModel.findCommand(typed)
        if (rec.name === undefined
                || (!cfg.consoleModel.showCheats && rec.cheat)) {
            revertEdit()
            return
        }
        if (commandCombo.find(typed, Qt.MatchFixedString) < 0) {
            // hidden by the filters: switch the dropdowns to the command's own
            // category/platform - the most restrictive options that match it
            applyCategoryFilter(rec.context)
            applyPlatformFilter(rec.platform)
        }
        const idx = commandCombo.find(typed, Qt.MatchFixedString)
        if (idx >= 0) {
            commandCombo.currentIndex = idx
            commandCombo.popup.close()
            suggestionPopup.close()
            suggestionIndex = -1
        } else {
            revertEdit()
        }
    }

    /// The console line the Build-A-Bind editors produce (without the surrounding
    /// bind). Empty when nothing is selected or no value is ready.
    function innerCommand() {
        if (!commandInfo)
            return ""
        if (commandInfo.isCvar && modeToggle.checked && toggleAvailable) {
            // A bool cvar toggles straight to TF2's `bindtoggle`, whose row is a
            // `bindtoggle` bind rather than a wrapped `incrementvar` command. The
            // numeric path keeps incrementvar so it can step through a range.
            if (commandInfo.input === "bool")
                return commandInfo.name
            return "incrementvar " + commandInfo.name + " " + commandInfo.min + " "
                    + commandInfo.max + " " + stepField.text
        }
        let parts = [commandInfo.name]
        for (let i = 0; i < argModel.count; ++i) {
            const arg = argModel.get(i)
            if (arg.optional && !arg.included)
                continue
            if (arg.value === "")
                continue
            parts.push(arg.value)
        }
        return parts.join(" ")
    }

    /// Push the generated command into the shared Custom Command buffer. The buffer
    /// holds the bare command - the same thing cfg.binds reports and OK applies -
    /// because cfg.setBind adds the `bind "<key>"` prefix itself; putting a full bind
    /// line here produced `bind "w" bind "w" +forward` on disk.
    function regen() {
        if (!commandInfo || dialog.customPanelActive)
            return // a custom panel is the author; the generator must not stomp it
        customCommand.text = innerCommand()
    }

    Connections {
        target: argModel
        function onDataChanged() { regen() }
    }

    // The combo boxes display "client controls" / "universal" from the start, so the
    // model filters must say the same thing or the pane shows entries the labels hide.
    Component.onCompleted: {
        cfg.consoleModel.contextFilter = categoryCombo.currentText === "all" ? "" : categoryCombo.currentText
        cfg.consoleModel.platformFilter = platformCombo.currentText === "all" ? "" : platformCombo.currentText
    }

    /// Open the dialog for a TF2 key token: shows the bind the key carries today, if any.
    function openForKey(token) {
        dialog.keyToken = token
        const command = dialog.boundCommand
        const verb = dialog.cfgRef.bindingKind(token.toLowerCase()) === "bindtoggle"
                        ? "bindtoggle" : "bind"
        dialog.originalBind = verb + ' "' + token + '"' + (command === "" ? "" : ' "' + command + '"')
        dialog.open()
    }

    title: "Dialog"          // parity P30: the widget's dialog title really is "Dialog"
    modal: true              // parity P36: widget used exec()
    // Size grows and shrinks with the content (the voicemenu panel is taller than
    // the old fixed box), capped at the window so a too-small window scrolls the
    // argument pane instead of overflowing. A plain QML Dialog has no resize
    // handles, so dynamic sizing is never user-resizable.
    width: 640
    readonly property real dialogChrome:
        topPadding + bottomPadding + (header ? header.implicitHeight : 0)
        + (footer ? footer.implicitHeight : 0)
    height: dialog.parent
            ? Math.min(dialogContent.implicitHeight + dialogChrome,
                       dialog.parent.height - 2 * Theme.spacing)
            : dialogContent.implicitHeight + dialogChrome
    padding: Theme.spacingWide
    font.family: Theme.familyUi
    font.pointSize: Theme.sizeUi
    standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    // Keep it modal like the widget's exec(): Escape rejects, clicking outside does not.
    closePolicy: Popup.CloseOnEscape

    onOpened: {
        existingBindField.text = dialog.originalBind
        // Old one-argument build/destroy forms migrate here; OK commits the
        // modernized text, Cancel keeps the bound command untouched.
        customCommand.text = modernizedCommand(dialog.boundCommand)
        // Pre-select the verb from the binding the key carries today.
        bindToggleCheck.checked = dialog.boundCommand !== ""
                                 && cfg.bindingKind(dialog.keyToken.toLowerCase()) === "bindtoggle"
        // Start each edit from a clean Build-A-Bind pane; the chosen category and
        // platform filters persist across opens on purpose.
        commandCombo.currentIndex = -1
        suggestionList = []
        suggestionIndex = -1
        suggestionPopup.close()
        // Reflect the bound command in Build-A-Bind when possible.
        prefillFromCommand()
        // Center on the window each time it opens (a plain Dialog sits at 0,0).
        if (dialog.parent) {
            dialog.x = Math.max(0, Math.round((dialog.parent.width - dialog.width) / 2))
            dialog.y = Math.max(0, Math.round((dialog.parent.height - dialog.height) / 2))
        }
    }
    // Deviation D16: the widget's accept slot was empty (P35), which made the keyboard a
    // viewer. OK now applies the command authored in the Custom Command tab: a non-empty
    // command becomes bind "<key>" "<command>", an empty one removes the key's bind.
    // Either way the cap repaints, because cfg.binds notifies. Nothing is written to the
    // .cfg file - the app has no save yet (docs/legacy/ui-qml-rewrite.md section 10).
    onAccepted: {
        const command = customCommand.text.trim()
        if (command === "")
            cfg.clearBind(dialog.keyToken)
        else if (bindToggleCheck.checked)
            cfg.setBindToggle(dialog.keyToken, command)
        else
            cfg.setBind(dialog.keyToken, command)
    }

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
        id: dialogContent
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
                // Reports the bind the selected key carries. The command to apply is
                // authored in the Custom Command tab, so this field is display only (D16).
                readOnly: true
                // W4: name the field by the key it reports on.
                Accessible.name: "Existing bind for " + dialog.keyToken
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

            // Build-A-Bind: pick a command from the allcmds.csv catalog (filtered by
            // context and platform), then let the input-type signature drive argument
            // editors. The generated bind lands in the Custom Command buffer, so both
            // tabs apply through the same OK path. Design: docs/legacy/allcmds-v5-migration.md.
            ColumnLayout {
                spacing: Theme.spacing

                // -- filters -------------------------------------------------------
                RowLayout {
                    spacing: Theme.spacing

                    Label {
                        text: "Category:"
                        color: Theme.colorText
                        font.family: Theme.familyUi
                        font.pointSize: Theme.sizeUi
                    }
                    Tf2ComboBox {
                        id: categoryCombo
                        objectName: "categoryCombo"
                        Layout.minimumWidth: 150
                        Layout.preferredWidth: 150
                        model: ["client controls", "client settings", "client render",
                                "server commands", "server settings", "engine", "all"]
                        currentIndex: 0
                        onActivated: function (index) {
                            cfg.consoleModel.contextFilter =
                                    currentText === "all" ? "" : currentText
                        }
                    }

                    Label {
                        text: "Platform:"
                        color: Theme.colorText
                        font.family: Theme.familyUi
                        font.pointSize: Theme.sizeUi
                    }
                    Tf2ComboBox {
                        id: platformCombo
                        objectName: "platformCombo"
                        Layout.minimumWidth: 150
                        Layout.preferredWidth: 150
                        model: ["universal", "linux", "windows", "xbox", "all"]
                        currentIndex: 0
                        onActivated: function (index) {
                            cfg.consoleModel.platformFilter =
                                    currentText === "all" ? "" : currentText
                        }
                    }

                    // Cheats filter: off hides "cheat"-flagged commands from the
                    // dropdown and the suggestions.
                    CheckBox {
                        id: cheatsCheck
                        objectName: "cheatsCheck"
                        text: "Cheats"
                        checked: true
                        onToggled: cfg.consoleModel.showCheats = checked
                    }
                }

                // -- command --------------------------------------------------------
                RowLayout {
                    spacing: Theme.spacing

                    Label {
                        text: "Command:"
                        color: Theme.colorText
                        font.family: Theme.familyUi
                        font.pointSize: Theme.sizeUi
                    }
                    Tf2ComboBox {
                        id: commandCombo
                        objectName: "commandCombo"
                        Layout.fillWidth: true
                        placeholderText: "< choose or type a command >"
                        textRole: "name"
                        model: cfg.consoleModel

                        // Editable command field. The contentItem override is per
                        // instance on purpose: the shared Tf2ComboBox stays read-only
                        // for every other dropdown. It must be declared BEFORE
                        // editable - the engine wires its text sync and inline
                        // auto-complete inside setEditable(), which inspects whatever
                        // contentItem exists at that moment.
                        contentItem: TextField {
                            leftPadding: Theme.spacing
                            rightPadding: Theme.spacing * 3
                            text: commandCombo.editText
                            placeholderText: commandCombo.placeholderText
                            autoScroll: true
                            selectByMouse: true
                            inputMethodHints: commandCombo.inputMethodHints
                            font: commandCombo.font
                            color: Theme.colorText
                            placeholderTextColor: Theme.colorTextMuted
                            selectionColor: Theme.colorBorderStrong
                            selectedTextColor: Theme.colorText
                            verticalAlignment: Text.AlignVCenter
                            background: null

                            // Up/Down pick suggestion entries and Enter finalizes
                            // them; without this the keys would escape the field and
                            // the combo would jump its plain list selection. Consuming
                            // here stops that propagation.
                            Keys.onDownPressed: function (event) {
                                event.accepted = moveSuggestion(1)
                            }
                            Keys.onUpPressed: function (event) {
                                event.accepted = moveSuggestion(-1)
                            }
                            Keys.onReturnPressed: function (event) {
                                // Browsing the real dropdown: let the combo's own
                                // Return handling activate the highlighted entry.
                                if (commandCombo.popup.opened)
                                    return
                                // Owned here so the rules are unconditional: a
                                // highlighted suggestion finalizes, everything else
                                // settles (commit a valid full command, else reset to
                                // the previous good one). The engine's acceptInput()
                                // never sees the key.
                                event.accepted = true
                                if (suggestionPopup.opened && suggestionIndex >= 0)
                                    commitSuggestion(suggestionList[suggestionIndex])
                                else
                                    settleCommand()
                            }
                        }
                        editable: true

                        onEditTextChanged: updateSuggestions()

                        onCurrentIndexChanged: rebuildArgs()

                        // Popup clicks and built-in commit paths land here; settling
                        // is idempotent, so revalidating costs nothing.
                        onActivated: function (index) { settleCommand() }
                        onAccepted: settleCommand()
                        // Leaving the field settles it, so uncommitted text never
                        // lingers past the next click. Skip while the popup has focus.
                        onActiveFocusChanged: if (!activeFocus && !popup.opened) settleCommand()

                        // Live-filtered suggestion pop-down shown while typing:
                        // whole-catalog contains-matches, so typed text reaches
                        // commands the category/platform filters hide.
                        // Shows only first 8 entries while typing; arrow keys / mouse wheel
                        // switch to full list with scroll bar.
                        Popup {
                            id: suggestionPopup
                            parent: commandCombo
                            x: 0
                            y: commandCombo.height
                            width: commandCombo.width
                            padding: 1
                            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                            // Max height = 8 items worth so the list scrolls when browsing.
                            readonly property real itemHeight: Theme.sizeUi + Theme.spacingTight * 2
                            readonly property real maxHeight: itemHeight * 8 + 2
                            height: Math.min(maxHeight, suggestionView.contentHeight)

                            background: Rectangle {
                                color: Theme.colorSurfaceSunken
                                border.width: Theme.borderWidth
                                border.color: Theme.colorBorderStrong
                                radius: Theme.radius
                            }

                            contentItem: ListView {
                                id: suggestionView
                                clip: true
                                width: parent.width
                                height: parent.height
                                // When browsing, show all; otherwise cap at 8.
                                model: browsing ? suggestionList : suggestionList.slice(0, 8)
                                currentIndex: suggestionIndex
                                ScrollIndicator.vertical: ScrollIndicator {
                                    // Themed like the command list's scroll bar.
                                    contentItem: Rectangle {
                                        implicitWidth: Theme.spacingTight + 2
                                        radius: implicitWidth / 2
                                        color: Theme.colorBorderStrong
                                    }
                                    visible: suggestionView.contentHeight > suggestionView.height
                                }
                                // Mouse wheel on the list enters browsing mode.
                                WheelHandler {
                                    onWheel: {
                                        if (!browsing) {
                                            dialog.browsing = true
                                        }
                                    }
                                }

                                delegate: ItemDelegate {
                                    id: suggestionItem
                                    required property var modelData

                                    width: ListView.view.width
                                    text: suggestionItem.modelData
                                    font.family: Theme.familyUi
                                    font.pointSize: Theme.sizeUi
                                    hoverEnabled: true
                                    onClicked: {
                                        commandCombo.editText = suggestionItem.modelData
                                        settleCommand()
                                        suggestionPopup.close()
                                    }

                                    contentItem: Text {
                                        text: suggestionItem.text
                                        font: suggestionItem.font
                                        color: Theme.colorText
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                        leftPadding: Theme.spacing
                                    }

                                    background: Rectangle {
                                        color: suggestionItem.hovered
                                               || suggestionItem.ListView.isCurrentItem
                                               ? Theme.colorSurfaceRaised : "transparent"
                                    }
                                }
                            }
                        }
                    }
                }

                Tf2Label {
                    Layout.fillWidth: true
                    text: commandInfo ? commandInfo.docs : ""
                    color: Theme.colorTextMuted
                    font.pointSize: Theme.sizeSmall
                    wrapMode: Text.WordWrap
                    visible: text !== ""
                }

                // -- cheat notice ----------------------------------------------------
                Tf2Label {
                    Layout.fillWidth: true
                    text: commandInfo && commandInfo.cheat
                          ? "Requires sv_cheats 1 to run." : ""
                    color: Theme.colorTextMuted
                    font.pointSize: Theme.sizeSmall
                    visible: text !== ""
                }

                // -- arguments -------------------------------------------------------
                Label {
                    text: "Arguments:"
                    color: Theme.colorText
                    font.family: Theme.familyUi
                    font.pointSize: Theme.sizeUi
                }

                ScrollView {
                    id: argsScroll
                    // Track the content: a bare ScrollView floors at its default
                    // implicit size, which left a large empty block behind the few
                    // small editors a no-argument command shows.
                    implicitHeight: argsContent.implicitHeight
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ScrollBar.horizontal: ScrollBar {
                        id: argsBar
                        policy: ScrollBar.AsNeeded
                        hoverEnabled: true
                        contentItem: Rectangle {
                            implicitHeight: Theme.spacingTight + 2
                            radius: implicitHeight / 2
                            color: argsBar.pressed ? Theme.colorAccent
                                  : argsBar.hovered ? Theme.colorTextMuted
                                  : Theme.colorBorderStrong
                        }
                    }

                    ColumnLayout {
                        id: argsContent
                        width: argsScroll.availableWidth
                        spacing: Theme.spacingTight

                        // Custom panel for commands with particular authoring
                        // (voicemenu today); replaces the generic editors below.
                        Item {
                            id: panelSlot
                            Layout.fillWidth: true
                            // Loader's implicit size is read-only and does not reset
                            // when the item unloads, which kept the dialog at the
                            // panel's height after switching away; the slot derives
                            // the height itself instead.
                            implicitHeight: dialog.customPanelActive && customPanelLoader.item
                                            ? customPanelLoader.item.implicitHeight : 0

                            Loader {
                                id: customPanelLoader
                                width: parent.width
                                height: item ? item.implicitHeight : 0
                                active: dialog.customPanelActive
                            sourceComponent: dialog.customPanelActive
                                             ? dialog.customPanels[dialog.activePanelName].component
                                             : null
                            onLoaded: {
                                item.authored.connect(function (text) {
                                    customCommand.text = text
                                })
                            }
                            }
                        }

                        // Generic editors - hidden while a custom panel is active.
                        ColumnLayout {
                            visible: !dialog.customPanelActive
                            spacing: Theme.spacingTight

                        // Toggle-vs-fixed choice for cvars that can be cycled with
                        // incrementvar (bool, or int/flt with a known range).
                        RowLayout {
                            visible: commandInfo && commandInfo.isCvar && toggleAvailable
                            spacing: Theme.spacingWide

                            ButtonGroup { id: cvarMode }

                            RadioButton {
                                id: modeToggle
                                text: "Toggle each press"
                                checked: true
                                ButtonGroup.group: cvarMode
                                onCheckedChanged: {
                                    regen()
                                    if (commandInfo && commandInfo.isCvar && commandInfo.input === "bool")
                                        bindToggleCheck.checked = checked
                                }
                            }
                            RadioButton {
                                text: "Set fixed value"
                                ButtonGroup.group: cvarMode
                                onCheckedChanged: regen()
                            }

                            Label {
                                text: "Step:"
                                visible: modeToggle.checked
                                color: Theme.colorText
                                font.family: Theme.familyUi
                                font.pointSize: Theme.sizeUi
                            }
                            TextField {
                                id: stepField
                                visible: modeToggle.checked
                                text: commandInfo && commandInfo.input === "flt" ? "0.1" : "1"
                                onTextEdited: regen()
                            }
                        }

                        Repeater {
                            model: ListModel { id: argModel }

                            delegate: RowLayout {
                                required property var model
                                required property int index

                                Layout.fillWidth: true
                                spacing: Theme.spacing

                                CheckBox {
                                    text: "include"
                                    checked: model.included
                                    visible: model.optional
                                    onToggled: model.included = checked
                                }

                                Label {
                                    text: model.label
                                    color: Theme.colorText
                                    font.family: Theme.familyUi
                                    font.pointSize: Theme.sizeUi
                                }

                                CheckBox {
                                    checked: model.value === "1"
                                    visible: model.kind === "bool"
                                    onToggled: model.value = checked ? "1" : "0"
                                }
                                TextField {
                                    visible: model.kind === "int"
                                    text: model.value
                                    font.family: Theme.familyMono
                                    font.pointSize: Theme.sizeMono
                                    onTextEdited: model.value = text
                                    validator: IntValidator {
                                        bottom: validBottom(model.min)
                                        top: validTop(model.max)
                                    }
                                }
                                TextField {
                                    visible: model.kind === "flt"
                                    text: model.value
                                    font.family: Theme.familyMono
                                    font.pointSize: Theme.sizeMono
                                    onTextEdited: model.value = text
                                    validator: DoubleValidator {
                                        bottom: validBottom(model.min)
                                        top: validTop(model.max)
                                        notation: DoubleValidator.StandardNotation
                                    }
                                }
                                TextField {
                                    visible: model.kind === "str"
                                    text: model.value
                                    Layout.fillWidth: true
                                    font.family: Theme.familyMono
                                    font.pointSize: Theme.sizeMono
                                    onTextEdited: model.value = text
                                }
                            }
                        }

                        Tf2Label {
                            text: commandInfo && commandInfo.input === "none"
                                  ? "This command takes no arguments." : ""
                            color: Theme.colorTextMuted
                            visible: text !== ""
                        }
                        }
                    }
                }
            }

            // Parity P34: plain text command editor, monospace, no wrapping.
            ScrollView {
                Layout.fillWidth: true
                // Same content tracking as argsScroll: no default-size floor.
                implicitHeight: customCommand.implicitHeight

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

        RowLayout {
            Layout.fillWidth: true

            CheckBox {
                id: bindToggleCheck
                text: "Save as bindtoggle (TF2 `bindtoggle` syntax)"
                checked: dialog.boundCommand !== ""
                         && dialog.cfgRef.bindingKind(dialog.keyToken.toLowerCase()) === "bindtoggle"
                enabled: commandInfo !== null
                         ? (commandInfo.isCvar && commandInfo.input === "bool")
                         : (dialog.boundCommand !== ""
                            && dialog.cfgRef.bindingKind(dialog.keyToken.toLowerCase()) === "bindtoggle")
                ToolTip.visible: hovered
                ToolTip.text: enabled ? "Writes `bindtoggle \"key\" \"cvar\"` so TF2 toggles "
                                     + "the boolean cvar each press."
                                     : "bindtoggle applies to bool cvars; use Toggle for int/float."
            }

            Item { Layout.fillWidth: true }
        }

        // Says what OK does, because neither tab did anything in the widget build and
        // "leave it empty to remove" is not something a user can guess (D16).
        Tf2Label {
            Layout.fillWidth: true
            text: dialog.keyToken === "" ? ""
                                        : qsTr('Binds this command to "%1". Clear the field to remove the bind.')
                                              .arg(dialog.keyToken)
            color: Theme.colorTextMuted
            font.pointSize: Theme.sizeSmall
            wrapMode: Text.WordWrap
        }
    }
}
