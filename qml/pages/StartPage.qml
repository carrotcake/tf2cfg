pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import tf2cfg
import "../components"
import "../theme"

// Start screen. Parity P2-P11 in docs/legacy/ui-qml-rewrite.md.
Item {
    id: page

    /// Selected config path: an absolute file path from Browse, or a ":/cfg/..." preset.
    property string cfgPath: ""
    property int presetIndex: -1
    property string errorText: ""

    /// Emitted after a successful load; Main.qml switches to the editor.
    signal loaded()

    /// "file:///C:/..." -> "C:/..." and "file:///home/user/x.cfg" -> "/home/user/x.cfg";
    /// anything already local passes through untouched.
    function localPath(url) {
        let s = url.toString()
        if (s.indexOf("file:") === 0) {
            s = s.replace(/^file:\/\//, "")
            // A filename may hold a bare % that is not an escape, which makes
            // decodeURIComponent throw; the undecoded string is still a usable path.
            try {
                s = decodeURIComponent(s)
            } catch (e) {
            }
            // The URL has an empty authority, so what is left starts with a slash the
            // path itself does not have - but only for a Windows drive letter. A Unix
            // path is "/home/..." and that leading slash is part of it.
            if (/^\/[A-Za-z]:/.test(s))
                s = s.slice(1)
        }
        return s
    }

    function selectPath(path) {
        page.cfgPath = path
        page.presetIndex = -1
        page.errorText = ""
    }

    function loadSelected() {
        page.errorText = ""
        Cfg.loadCfg(page.cfgPath)
    }

    Connections {
        target: Cfg
        function onLoadedChanged() {
            // A failed load leaves the previously parsed rows in place, so Cfg.loaded is
            // still true and the success branch would swallow the error.
            if (Cfg.lastError !== "")
                page.errorText = "Could not load " + page.cfgPath + " (" + Cfg.lastError + ")"
            else if (Cfg.loaded)
                page.loaded()
        }
        function onLastErrorChanged() {
            if (Cfg.lastError !== "" && !Cfg.loaded) {
                page.errorText = "Could not load " + page.cfgPath + " (" + Cfg.lastError + ")"
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
                        objectName: "cfgPathField"
                        text: page.cfgPath
                        readOnly: true
                        placeholderText: "< Path To TF2 config.cfg >"
                        // W4: the field's only visible name is its placeholder.
                        Accessible.name: "TF2 config.cfg path"
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
                    model: Cfg.presets
                    textRole: "label"
                    placeholderText: "< Choose a Preset >"
                    currentIndex: page.presetIndex
                    onActivated: function (index) {
                        page.presetIndex = index
                        page.cfgPath = Cfg.presetAt(index)
                        page.errorText = ""
                    }
                }

                Tf2Label {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.panelMaxWidth
                    Layout.alignment: Qt.AlignHCenter
                    visible: page.presetIndex >= 0
                    text: Cfg.presetNote(page.presetIndex)
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
