import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"
import "../theme"

// Start screen. Parity P2-P11 in docs/legacy/ui-qml-rewrite.md.
Item {
    id: page

    // Window sizing contract with Main.qml (parity P2 / deviation D2).
    readonly property int preferredWidth: Theme.startWidth
    readonly property int preferredHeight: Theme.startHeight
    readonly property string pageTitle: "TF2 Config Manager"

    /// Selected config path: an absolute file path from Browse, or a ":/cfg/..." preset.
    property string cfgPath: ""
    property int presetIndex: -1
    property string errorText: ""

    /// Emitted after a successful load; Main.qml switches to the editor.
    signal loaded()

    /// "file:///C:/..." -> "C:/..."; anything already local passes through untouched.
    function localPath(url) {
        let s = url.toString()
        if (s.indexOf("file:") === 0)
            s = decodeURIComponent(s.replace(/^file:\/{2,3}/, ""))
        return s
    }

    function selectPath(path) {
        page.cfgPath = path
        page.presetIndex = -1
        page.errorText = ""
    }

    function loadSelected() {
        page.errorText = ""
        cfg.loadCfg(page.cfgPath)
    }

    Connections {
        target: cfg
        function onLoadedChanged() {
            if (cfg.loaded) {
                page.loaded()
            } else if (cfg.lastError.length > 0) {
                page.errorText = "Could not load " + page.cfgPath + " (" + cfg.lastError + ")"
            }
        }
        function onLastErrorChanged() {
            if (cfg.lastError.length > 0 && !cfg.loaded) {
                page.errorText = "Could not load " + page.cfgPath + " (" + cfg.lastError + ")"
            }
        }
    }

    // Parity P6: the only file dialog in the app, filtered to *.cfg.
    FileDialog {
        id: fileDialog
        title: "Open TF2 Configuration File"
        nameFilters: ["Source Config File (*.cfg)"]
        onAccepted: {
            if (fileDialog.selectedFile.toString().length > 0)
                page.selectPath(page.localPath(fileDialog.selectedFile))
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: Theme.spacingWide

        // -- banner (P3) -------------------------------------------------------
        Panel {
            Layout.fillWidth: true
            Layout.preferredHeight: 104

            Label {
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: "TF2 Config Manager"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.family: Theme.familyBanner
                font.pointSize: Theme.sizeBanner
                color: Theme.colorText
            }
        }

        // -- file or preset (P4-P10) ------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacing

            Panel {
                Layout.preferredWidth: Theme.panelMaxWidth
                Layout.maximumWidth: Theme.panelMaxWidth
                Layout.fillHeight: true
                title: "Open your TF2 Configuration File..."

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacing

                    Tf2TextField {
                        Layout.fillWidth: true
                        text: page.cfgPath
                        readOnly: true
                        placeholderText: "< Path To TF2 config.cfg >"
                    }

                    Tf2Button {
                        text: "Browse"
                        onClicked: fileDialog.open()
                    }
                }

                Item { Layout.fillHeight: true }
            }

            Panel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: "... Or Choose From a Preset"

                Tf2ComboBox {
                    id: presetCombo
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.panelMaxWidth
                    Layout.alignment: Qt.AlignHCenter
                    model: cfg.presets
                    textRole: "label"
                    placeholderText: "< Choose a Preset >"
                    currentIndex: page.presetIndex
                    onActivated: function (index) {
                        page.presetIndex = index
                        page.cfgPath = cfg.presetAt(index)
                        page.errorText = ""
                    }
                }

                Tf2Label {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.panelMaxWidth
                    Layout.alignment: Qt.AlignHCenter
                    visible: page.presetIndex >= 0
                    text: cfg.presetNote(page.presetIndex)
                    color: Theme.colorTextMuted
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }

                Item { Layout.fillHeight: true }
            }
        }

        // -- go (P11) ---------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing

            Tf2Button {
                text: "Load Selected Config"
                enabled: page.cfgPath.length > 0
                onClicked: page.loadSelected()
            }

            Tf2Label {
                Layout.fillWidth: true
                visible: page.errorText.length > 0
                text: page.errorText
                color: Theme.colorRed
                elide: Text.ElideMiddle
            }
        }
    }
}
