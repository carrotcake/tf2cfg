pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../components"
import "../../theme"

// Tab 1: the Config Editor (docs/DESIGN-config-editor.md). Master-detail over every
// parsed config line: rows on the left, an editor for the selection on the right.
// Binds redirect to Edit Binds (Chunk A = raw text editing; guided editors are Chunk B).
Panel {
    id: tab

    /// Context property handle for qualified access (avoids repeated advisories).
    readonly property var cfgRef: cfg

    /// Row the editor pane works on (a source-model row, not the filtered index).
    property int selectedRow: -1
    /// Cached rowInfo() for selectedRow, refreshed by the model signal below.
    property var selectedInfo: null
    /// True while the editor pane inserts a new row instead of editing the selection.
    property bool addMode: false
    /// Catalog record (findCommand) of the selected row's head, null when the row is
    /// unknown/raw/a bind - the guided pane (Chunk B) shows only for known commands.
    property var guidedRec: null
    /// The cvar's current value token (raw token, quotes included), one field.
    property string cvarValue: ""
    /// Chain editor state (Chunk C, §5): true when the selection is a compound line
    /// or an alias definition. chainName is the alias's name in body mode, "" when
    /// editing a plain compound line.
    property bool chainActive: false
    property string chainName: ""

    /// Requested by the redirect dialog: open this key's bind in Edit Binds.
    signal bindActivated(string keyToken)

    function refreshSelection() {
        selectedInfo = selectedRow >= 0 ? cfgRef.rowInfo(selectedRow) : null
        // Imperative, not a binding: the user's typing would break a text binding.
        lineEdit.text = addMode ? "" : (selectedInfo === null ? "" : selectedInfo.line)
        rebuildGuided()
        rebuildChain()
    }

    // -- guided pane (Chunk B, docs/DESIGN-config-editor.md §4) -------------------
    // shortcut: parseSignature/validBottom/validTop/defaultValueFor are duplicated
    // from BindEditorDialog; extract to a shared JS lib when a third consumer appears.

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

    function defaultValueFor(kind) {
        if (kind === "int")
            return "0"
        if (kind === "flt")
            return "0.0"
        return ""
    }

    /// Fill the guided editors from a command line. Called with the selection's line
    /// (selection change / Apply) or with the raw field's text (editingFinished) - the
    /// §8 reverse-parse: custom text drives the guided fields, through the same C++
    /// tokenizer the file parser uses.
    function rebuildGuided(text) {
        argModel.clear()
        guidedRec = null
        cvarValue = ""
        if (addMode || selectedInfo === null || selectedInfo.boundKey !== "")
            return
        const parsed = cfgRef.parseCommandLine(text !== undefined ? text
                                                                 : selectedInfo.line)
        if (!parsed.ok)
            return
        const parts = parsed.args
        const rec = cfgRef.consoleModel.findCommand(parts[0])
        if (rec.name === undefined)
            return // not in catalog: raw text only (§3)
        guidedRec = rec
        if (rec.isCvar) {
            cvarValue = parts.length > 1 ? parts[1] : ""
            cvarField.text = cvarValue
            return
        }
        const specs = parseSignature(rec.input)
        const argSpecs = rec.argsSpec || []
        specs.forEach(function (spec, i) {
            // Per-argument bounds from the catalog (docs/DESIGN-arg-bounds.md), the
            // same rule the bind dialog's rebuildArgs applies.
            const bs = argSpecs[i] && argSpecs[i].index === i ? argSpecs[i] : null
            const kind = bs ? (bs.isFloat ? "flt" : spec.kind) : spec.kind
            const token = parts[i + 1] !== undefined ? parts[i + 1] : ""
            argModel.append({
                kind: kind,
                optional: spec.optional,
                included: token !== "" || !spec.optional,
                label: "param " + (i + 1) + " (" + kind + ")",
                value: token !== "" ? token
                     : specs.length === 1 && rec.def !== "" ? rec.def
                     : defaultValueFor(kind),
                min: bs ? (bs.hasMin ? bs.min : "") : rec.min,
                max: bs ? (bs.hasMax ? bs.max : "") : rec.max
            })
        })
    }

    /// Regenerate the raw line from the guided editors and put it in the raw field -
    /// one commit path: Apply parses whatever the raw field holds (§4.2).
    function regenGuided() {
        if (guidedRec === null)
            return
        const parts = [guidedRec.name]
        if (guidedRec.isCvar) {
            if (cvarValue !== "")
                parts.push(cvarValue)
        } else {
            for (let i = 0; i < argModel.count; ++i) {
                const arg = argModel.get(i)
                if (arg.optional && !arg.included)
                    continue
                if (arg.value === "")
                    continue
                parts.push(arg.value)
            }
        }
        lineEdit.text = parts.join(" ")
    }

    // -- chain editor (Chunk C, §5) -----------------------------------------------
    // Compound lines (`a; b`) and alias definitions (`alias name "a; b"`) are edited
    // as a segment list; the joined text lands in the raw field and Apply commits it.

    function rebuildChain(text) {
        chainActive = false
        chainName = ""
        if (addMode || selectedInfo === null || selectedInfo.boundKey !== "")
            return
        const src = text !== undefined ? text : selectedInfo.line
        const parsed = cfgRef.parseCommandLine(src)
        if (!parsed.ok)
            return
        const parts = parsed.args
        if (parts[0].toLowerCase() === "alias") {
            // Body mode: the body is one quoted token whose text splits into segments.
            if (parts.length < 2)
                return
            chainName = cfgRef.unquoteToken(parts[1])
            const body = parts.length >= 3 ? cfgRef.unquoteToken(parts[2]) : ""
            chainActive = true
            chainEditor.setSegments(cfgRef.splitSegments(body))
            return
        }
        const segs = cfgRef.splitSegments(src)
        if (segs.length > 1) {
            chainActive = true
            chainEditor.setSegments(segs)
        }
    }

    function regenChain() {
        if (!chainActive)
            return
        const segs = chainEditor.segmentList()
        if (chainName !== "") {
            // Re-quoted canonically (D19): name and body always quoted, escapes kept
            // lexer-correct by quoteToken.
            lineEdit.text = cfgRef.quoteToken(chainName) + " "
                            + cfgRef.quoteToken(segs.join("; "))
        } else {
            lineEdit.text = segs.join("; ")
        }
    }

    onSelectedRowChanged: {
        addMode = false
        refreshSelection()
    }

    // Any row change (Apply, delete, save resync) refreshes the cached selection.
    Connections {
        target: tab.cfgRef.commandModel
        function onDataChanged() { tab.refreshSelection() }
        function onModelReset() { tab.selectedRow = -1 }
        function onRowsRemoved() { tab.refreshSelection() }
    }

    function selectRow(sourceRow, boundKey) {
        selectedRow = sourceRow
        if (boundKey !== "")
            redirectDialog.open()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        // -- search + type filter (§2) ----------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing

            Tf2TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "< filter rows >"
                // W4: the field's only visible name is its placeholder.
                Accessible.name: "Config Editor row filter"
                onTextChanged: tab.cfgRef.rowFilter.filterText = text
            }

            Tf2ComboBox {
                id: kindCombo
                Layout.preferredWidth: Theme.comboDefaultWidth
                model: ["All types", "Cvars", "Commands", "Aliases", "Not in catalog", "Binds"]
                // Index 0 = all; the proxy's kinds are 0..4, so the mapping is index - 1.
                onActivated: function (index) {
                    tab.cfgRef.rowFilter.filterKind = index - 1
                }
            }
        }

        // -- master-detail ------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacing

            // -- left: the row list -------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                color: Theme.colorSurfaceSunken
                border.width: Theme.borderWidth
                border.color: Theme.colorBorder
                radius: Theme.radius
                clip: true

                ListView {
                    id: rowList
                    anchors.fill: parent
                    anchors.margins: Theme.spacingTight
                    clip: true
                    reuseItems: true
                    model: tab.cfgRef.rowFilter
                    ScrollBar.vertical: ScrollBar {
                        id: rowScroll
                        policy: ScrollBar.AsNeeded
                        hoverEnabled: true
                        contentItem: Rectangle {
                            implicitWidth: Theme.spacingTight + 2
                            radius: implicitWidth / 2
                            color: rowScroll.pressed ? Theme.colorAccent
                                  : rowScroll.hovered ? Theme.colorTextMuted
                                  : Theme.colorBorderStrong
                        }
                    }

                    delegate: Rectangle {
                        id: rowDelegate

                        required property int sourceRow
                        required property string line
                        required property string typeName
                        required property string boundKey
                        required property string warning

                        readonly property bool isSelected:
                            tab.selectedRow === rowDelegate.sourceRow

                        width: rowList.width
                        height: rowText.implicitHeight + Theme.spacingTight * 2
                        color: rowDelegate.isSelected ? Theme.colorKeyCapPressed
                              : rowHover.hovered ? Theme.colorKeyCapHover
                              : "transparent"
                        border.width: rowDelegate.isSelected ? Theme.borderWidth : 0
                        border.color: Theme.colorAccent
                        radius: Theme.radius

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spacingTight
                            anchors.rightMargin: Theme.spacingTight
                            spacing: Theme.spacingTight

                            // type badge (§3)
                            Rectangle {
                                implicitWidth: badgeText.implicitWidth + Theme.spacingTight * 2
                                implicitHeight: badgeText.implicitHeight
                                radius: Theme.radius
                                color: Theme.colorSurfaceRaised
                                visible: rowDelegate.typeName !== ""

                                Text {
                                    id: badgeText
                                    anchors.centerIn: parent
                                    text: rowDelegate.boundKey !== "" ? "bind"
                                          : rowDelegate.line.toLowerCase().startsWith("alias ") ? "alias"
                                          : rowDelegate.typeName === "command" ? "cmd"
                                          : rowDelegate.typeName
                                    color: rowDelegate.boundKey !== "" ? Theme.colorRed
                                          : Theme.colorTextMuted
                                    font.family: Theme.familyUi
                                    font.pointSize: Theme.sizeSmall
                                }
                            }

                            Text {
                                id: rowText
                                Layout.fillWidth: true
                                text: rowDelegate.line
                                color: Theme.colorText
                                font.family: Theme.familyMono
                                font.pointSize: Theme.sizeMono
                                elide: Text.ElideRight
                            }

                            // §7 replay-order warning
                            Text {
                                visible: rowDelegate.warning !== ""
                                text: "!"
                                color: Theme.colorRed
                                font.family: Theme.familyUi
                                font.pointSize: Theme.sizeUi
                                font.bold: true

                                ToolTip.visible: rowHover.hovered && rowDelegate.warning !== ""
                                ToolTip.text: rowDelegate.warning
                            }
                        }

                        HoverHandler { id: rowHover; cursorShape: Qt.PointingHandCursor }

                        // W2 pattern: tab-reachable rows, Return/Space select, focus
                        // shows the hover highlight.
                        TapHandler {
                            onTapped: tab.selectRow(rowDelegate.sourceRow,
                                                    rowDelegate.boundKey)
                        }
                        activeFocusOnTab: true
                        Keys.onReturnPressed: tab.selectRow(rowDelegate.sourceRow,
                                                            rowDelegate.boundKey)
                        Keys.onSpacePressed: tab.selectRow(rowDelegate.sourceRow,
                                                           rowDelegate.boundKey)

                        // W4: the row's name is its text; the warning is the description.
                        Accessible.role: Accessible.ListItem
                        Accessible.name: rowDelegate.line
                        Accessible.description: rowDelegate.warning
                        Accessible.onPressAction: tab.selectRow(rowDelegate.sourceRow,
                                                                rowDelegate.boundKey)
                    }

                    Tf2Label {
                        anchors.centerIn: parent
                        visible: rowList.count === 0
                        text: tab.cfgRef.loaded ? "No rows match the filter."
                                                : "No config loaded."
                        color: Theme.colorTextMuted
                    }
                }

                // "+ add" opens the editor pane in insert mode (§9: after the
                // selected row, or end of file when nothing is selected).
                Tf2Button {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: Theme.spacing
                    text: "+ add"
                    onClicked: {
                        tab.addMode = true
                        lineEdit.text = ""
                        editorPane.forceEditorFocus()
                    }
                }
            }

            // -- right: the editor pane (§4) -----------------------------------
            Rectangle {
                id: editorPane
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                color: Theme.colorSurfaceSunken
                border.width: Theme.borderWidth
                border.color: Theme.colorBorder
                radius: Theme.radius

                function forceEditorFocus() { lineEdit.forceActiveFocus() }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacing
                    spacing: Theme.spacing

                    // Selection header: type + command + doc/tooltip goes in Chunk B.
                    Tf2Label {
                        Layout.fillWidth: true
                        visible: tab.addMode || tab.selectedInfo !== null
                        text: tab.addMode ? "New row"
                              : tab.selectedInfo === null ? ""
                              : tab.selectedInfo.typeName + ": " + tab.selectedInfo.command
                        color: Theme.colorText
                        font.pointSize: Theme.sizeUi
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    // §7: the warning, spelled out where the edit happens.
                    Tf2Label {
                        Layout.fillWidth: true
                        visible: !tab.addMode && tab.selectedInfo !== null
                                 && tab.selectedInfo.warning !== ""
                        text: tab.selectedInfo === null ? "" : tab.selectedInfo.warning
                        color: Theme.colorRed
                        wrapMode: Text.WordWrap
                        font.pointSize: Theme.sizeSmall
                    }

                    // Bind rows are not editable here: redirect (§1).
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: tab.selectedInfo !== null
                                 && tab.selectedInfo.boundKey !== "" && !tab.addMode
                        spacing: Theme.spacing

                        Tf2Label {
                            Layout.fillWidth: true
                            text: "This key is bound. Binds are edited in the "
                                  + "Edit Binds tab."
                            color: Theme.colorTextMuted
                            wrapMode: Text.WordWrap
                        }

                        Tf2Button {
                            text: "Edit in the Bind Editor"
                            onClicked: redirectDialog.open()
                        }
                    }

                    // -- guided editors (§4.1): catalog-known, non-bind rows only --
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: tab.guidedRec !== null && !tab.addMode
                                 && !tab.chainActive
                        spacing: Theme.spacingTight

                        Tf2Label {
                            Layout.fillWidth: true
                            text: tab.guidedRec !== null ? tab.guidedRec.docs : ""
                            color: Theme.colorTextMuted
                            font.pointSize: Theme.sizeSmall
                            wrapMode: Text.WordWrap
                            visible: text !== ""
                        }

                        Tf2Label {
                            Layout.fillWidth: true
                            text: tab.guidedRec !== null && tab.guidedRec.cheat
                                  ? "Requires sv_cheats 1 to run." : ""
                            color: Theme.colorTextMuted
                            font.pointSize: Theme.sizeSmall
                            visible: text !== ""
                        }

                        // One cvar value, typed by the catalog: bool checkbox,
                        // int/flt field clamped to the catalog range, str token.
                        RowLayout {
                            Layout.fillWidth: true
                            visible: tab.guidedRec !== null && tab.guidedRec.isCvar
                            spacing: Theme.spacing

                            Label {
                                text: "Value:"
                                color: Theme.colorText
                                font.family: Theme.familyUi
                                font.pointSize: Theme.sizeUi
                            }

                            CheckBox {
                                visible: tab.guidedRec !== null
                                         && tab.guidedRec.input === "bool"
                                checked: tab.cvarValue === "1"
                                onToggled: {
                                    tab.cvarValue = checked ? "1" : "0"
                                    tab.regenGuided()
                                }
                                Accessible.name: "Value"
                            }

                            TextField {
                                id: cvarField
                                visible: tab.guidedRec !== null
                                         && tab.guidedRec.input !== "bool"
                                Layout.fillWidth: true
                                color: Theme.colorText
                                font.family: Theme.familyMono
                                font.pointSize: Theme.sizeMono
                                hoverEnabled: true
                                selectByMouse: true
                                onTextEdited: {
                                    tab.cvarValue = text
                                    tab.regenGuided()
                                }
                                // The token is edited verbatim (quotes included), so
                                // the lexer stays the only quoting authority.
                                validator: tab.guidedRec !== null
                                           && tab.guidedRec.input === "int" ? cvarIntV
                                           : tab.guidedRec !== null
                                             && tab.guidedRec.input === "flt" ? cvarFltV
                                           : null
                                IntValidator {
                                    id: cvarIntV
                                    bottom: tab.guidedRec !== null
                                            ? tab.validBottom(tab.guidedRec.min)
                                            : -2147483647
                                    top: tab.guidedRec !== null
                                         ? tab.validTop(tab.guidedRec.max)
                                         : 2147483647
                                }
                                DoubleValidator {
                                    id: cvarFltV
                                    notation: DoubleValidator.StandardNotation
                                    bottom: tab.guidedRec !== null
                                            ? tab.validBottom(tab.guidedRec.min)
                                            : -2147483647
                                    top: tab.guidedRec !== null
                                         ? tab.validTop(tab.guidedRec.max)
                                         : 2147483647
                                }
                                background: Rectangle {
                                    color: Theme.colorSurface
                                    radius: Theme.radius
                                    border.width: Theme.borderWidth
                                    border.color: cvarField.activeFocus ? Theme.colorAccent
                                                                        : Theme.colorBorder
                                }
                                Accessible.name: "Value"
                            }
                        }

                        Label {
                            text: "Arguments:"
                            visible: tab.guidedRec !== null && !tab.guidedRec.isCvar
                                     && tab.guidedRec.input !== "none"
                            color: Theme.colorText
                            font.family: Theme.familyUi
                            font.pointSize: Theme.sizeUi
                        }

                        // One editor per usage-signature token; optional tokens get an
                        // include checkbox. Same shape as the bind dialog's pane.
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
                                    onToggled: {
                                        model.included = checked
                                        tab.regenGuided()
                                    }
                                    Accessible.name: model.label
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
                                    onToggled: {
                                        model.value = checked ? "1" : "0"
                                        tab.regenGuided()
                                    }
                                    Accessible.name: model.label
                                }
                                TextField {
                                    visible: model.kind === "int"
                                    text: model.value
                                    font.family: Theme.familyMono
                                    font.pointSize: Theme.sizeMono
                                    onTextEdited: {
                                        model.value = text
                                        tab.regenGuided()
                                    }
                                    validator: IntValidator {
                                        bottom: tab.validBottom(model.min)
                                        top: tab.validTop(model.max)
                                    }
                                }
                                TextField {
                                    visible: model.kind === "flt"
                                    text: model.value
                                    font.family: Theme.familyMono
                                    font.pointSize: Theme.sizeMono
                                    onTextEdited: {
                                        model.value = text
                                        tab.regenGuided()
                                    }
                                    validator: DoubleValidator {
                                        bottom: tab.validBottom(model.min)
                                        top: tab.validTop(model.max)
                                        notation: DoubleValidator.StandardNotation
                                    }
                                }
                                TextField {
                                    visible: model.kind === "str"
                                    text: model.value
                                    Layout.fillWidth: true
                                    font.family: Theme.familyMono
                                    font.pointSize: Theme.sizeMono
                                    onTextEdited: {
                                        model.value = text
                                        tab.regenGuided()
                                    }
                                }
                            }
                        }

                        Tf2Label {
                            text: tab.guidedRec !== null && tab.guidedRec.input === "none"
                                  ? "This command takes no arguments." : ""
                            color: Theme.colorTextMuted
                            visible: text !== ""
                        }
                    }

                    // -- chain editor (§5): compound lines and alias bodies --
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: tab.chainActive && !tab.addMode
                        spacing: Theme.spacingTight

                        Tf2Label {
                            Layout.fillWidth: true
                            text: tab.chainName !== ""
                                  ? 'Alias "' + tab.chainName + '" - commands run in order:'
                                  : "Compound line - commands run in order:"
                            color: Theme.colorText
                            font.pointSize: Theme.sizeUi
                            font.bold: true
                            wrapMode: Text.WordWrap
                        }

                        ChainEditor {
                            id: chainEditor
                            Layout.fillWidth: true
                            onChanged: tab.regenChain()
                        }
                    }

                    // Raw text editor (§4.2); guided fields are Chunk B. The text is
                    // set imperatively by refreshSelection/Apply, never bound.
                    TextArea {
                        id: lineEdit
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: tab.addMode
                                 || (tab.selectedInfo !== null
                                     && tab.selectedInfo.boundKey === "")
                        color: Theme.colorText
                        selectionColor: Theme.colorAccent
                        selectedTextColor: Theme.colorAccentText
                        font.family: Theme.familyMono
                        font.pointSize: Theme.sizeMono
                        wrapMode: Text.WrapAnywhere
                        hoverEnabled: true
                        selectByMouse: true
                        // The bind dialog's Custom Command styling: dark and sunken.
                        background: Rectangle {
                            color: Theme.colorSurface
                            radius: Theme.radius
                            border.width: Theme.borderWidth
                            border.color: lineEdit.activeFocus ? Theme.colorAccent
                                                              : Theme.colorBorder
                        }
                        Accessible.name: "Command line editor"
                        Accessible.role: Accessible.EditableText
                        // §8 reverse-parse: committing raw text re-fills the guided
                        // fields from the same text, so both paths stay one truth.
                        onEditingFinished: {
                            tab.rebuildGuided(lineEdit.text)
                            tab.rebuildChain(lineEdit.text)
                        }
                    }

                    Item { Layout.fillHeight: true }

                    // Error note: the parse refused the text.
                    Tf2Label {
                        id: errorNote
                        Layout.fillWidth: true
                        visible: errorNote.editorError !== ""
                        property string editorError: ""
                        text: errorNote.editorError
                        color: Theme.colorRed
                        wrapMode: Text.WordWrap
                        font.pointSize: Theme.sizeSmall
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: tab.addMode
                                 || (tab.selectedInfo !== null
                                     && tab.selectedInfo.boundKey === "")
                        spacing: Theme.spacing

                        Tf2Button {
                            text: tab.addMode ? "Insert" : "Apply"
                            onClicked: {
                                errorNote.editorError = ""
                                if (tab.addMode) {
                                    const row = tab.cfgRef.insertRowAfter(
                                        tab.selectedRow, lineEdit.text)
                                    if (row < 0) {
                                        errorNote.editorError = "not a valid command line"
                                        return
                                    }
                                    // Selecting the new row also renders its canonical
                                    // text (onSelectedRowChanged -> refreshSelection).
                                    tab.selectedRow = row
                                } else {
                                    if (!tab.cfgRef.setRowText(tab.selectedRow,
                                                               lineEdit.text)) {
                                        errorNote.editorError = "not a valid command line"
                                        return
                                    }
                                    // The same dataChanged refreshes the pane, so the
                                    // field now shows the canonical rendering: what
                                    // lands on disk is what the user reviews.
                                }
                            }
                        }

                        Tf2Button {
                            visible: !tab.addMode
                            text: "Revert"
                            flat: true
                            onClicked: {
                                errorNote.editorError = ""
                                lineEdit.text = tab.selectedInfo === null ? ""
                                                                          : tab.selectedInfo.line
                            }
                        }

                        Tf2Button {
                            visible: !tab.addMode
                            text: "Delete"
                            onClicked: {
                                errorNote.editorError = ""
                                const row = tab.selectedRow
                                tab.selectedRow = -1 // drop the selection first
                                tab.cfgRef.deleteRow(row)
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    // Empty state, mirroring the Command Overview's.
                    Tf2Label {
                        Layout.alignment: Qt.AlignHCenter
                        visible: !tab.addMode && tab.selectedInfo === null
                        text: tab.cfgRef.loaded
                              ? "Select a row to edit it, or press + add."
                              : "No config loaded."
                        color: Theme.colorTextMuted
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }

    // §1: binds redirect to Edit Binds. Yes switches the tab and opens the dialog.
    Dialog {
        id: redirectDialog
        modal: true
        closePolicy: Popup.CloseOnEscape
        anchors.centerIn: parent
        width: 420
        title: qsTr("This key is bound")
        standardButtons: Dialog.Yes | Dialog.No

        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr('Binds are edited in the Edit Binds tab. Open the editor for '
                       + 'the "%1" key there?').arg(
                      tab.selectedInfo === null ? "" : tab.selectedInfo.boundKey)
        }
        onAccepted: tab.bindActivated(
                        tab.selectedInfo === null ? "" : tab.selectedInfo.boundKey)
    }
}