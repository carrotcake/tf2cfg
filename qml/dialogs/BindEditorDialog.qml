import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"

// Bind editor. Parity P30-P36.
Dialog {
    id: dialog
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

    /// Toggle mode needs the engine's incrementvar command to know where to stop:
    /// bool cvars are always 0..1, int/flt need the CSV min/max.
    readonly property bool toggleAvailable:
        commandInfo !== null && commandInfo.isCvar
        && (commandInfo.input === "bool"
            || (commandInfo.min !== "" && commandInfo.max !== ""))

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

    function validBottom(bound) { return bound === "" ? -2147483647 : parseInt(bound) }
    function validTop(bound) { return bound === "" ? 2147483647 : parseInt(bound) }

    /// Rebuild the argument editors from the selected command's signature.
    function rebuildArgs() {
        commandInfo = commandCombo.currentIndex >= 0
                ? cfg.consoleModel.commandAt(commandCombo.currentIndex) : null
        argModel.clear()
        if (!commandInfo)
            return
        const specs = parseSignature(commandInfo.input)
        specs.forEach(function (spec, i) {
            argModel.append({
                kind: spec.kind,
                optional: spec.optional,
                included: !spec.optional,
                label: "param " + (i + 1) + " (" + spec.kind + ")",
                value: specs.length === 1 && commandInfo.def !== "" ? commandInfo.def
                                                                    : defaultValueFor(spec.kind),
                min: commandInfo.min,
                max: commandInfo.max
            })
        })
        regen()
    }

    function defaultValueFor(kind) {
        if (kind === "int")
            return "0"
        if (kind === "flt")
            return "0.0"
        return ""
    }

    /// The console line the Build-A-Bind editors produce (without the surrounding
    /// bind). Empty when nothing is selected or no value is ready.
    function innerCommand() {
        if (!commandInfo)
            return ""
        if (commandInfo.isCvar && modeToggle.checked && toggleAvailable) {
            if (commandInfo.input === "bool")
                return "incrementvar " + commandInfo.name + " 0 1 1"
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

    /// Push the generated bind into the shared Custom Command buffer.
    function regen() {
        if (!commandInfo)
            return
        const inner = innerCommand()
        customCommand.text = inner === "" ? "" : 'bind "' + dialog.keyToken + '" "' + inner + '"'
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
        dialog.originalBind = 'bind "' + token + '"' + (command === "" ? "" : ' "' + command + '"')
        dialog.open()
    }

    title: "Dialog"          // parity P30: the widget's dialog title really is "Dialog"
    modal: true              // parity P36: widget used exec()
    width: 640
    height: 460
    padding: Theme.spacingWide
    font.family: Theme.familyUi
    font.pointSize: Theme.sizeUi
    standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    // Keep it modal like the widget's exec(): Escape rejects, clicking outside does not.
    closePolicy: Popup.CloseOnEscape

    onOpened: {
        existingBindField.text = dialog.originalBind
        customCommand.text = dialog.boundCommand
        // Start each edit from a clean Build-A-Bind pane; the chosen category and
        // platform filters persist across opens on purpose.
        commandCombo.currentIndex = -1
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
    // .cfg file - the app has no save yet (docs/ui-qml-rewrite.md section 10).
    onAccepted: {
        const command = customCommand.text.trim()
        if (command === "")
            cfg.clearBind(dialog.keyToken)
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
            // tabs apply through the same OK path. Design: docs/allcmds-v5-migration.md.
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
                        model: ["universal", "linux", "windows", "xbox", "all"]
                        currentIndex: 0
                        onActivated: function (index) {
                            cfg.consoleModel.platformFilter =
                                    currentText === "all" ? "" : currentText
                        }
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
                        placeholderText: "< choose a command >"
                        textRole: "name"
                        model: cfg.consoleModel
                        onCurrentIndexChanged: rebuildArgs()
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
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                    ColumnLayout {
                        width: argsScroll.availableWidth
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
                                onCheckedChanged: regen()
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
