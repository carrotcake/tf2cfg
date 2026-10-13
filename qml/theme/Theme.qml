pragma Singleton

import QtQuick
pragma ComponentBehavior: Bound

// Design tokens for the whole UI. See docs/legacy/ui-qml-rewrite.md section 9.
// Aesthetic brief: dark, industrial, game-adjacent - the app is a companion tool for
// Team Fortress 2, and the game's own fonts are already bundled.
QtObject {
    // -- surfaces and lines --------------------------------------------------
    readonly property color colorBg: "#141719"
    readonly property color colorSurface: "#1d2124"
    readonly property color colorSurfaceRaised: "#262b2f"
    readonly property color colorSurfaceSunken: "#0f1214"
    readonly property color colorBorder: "#3a4045"
    readonly property color colorBorderStrong: "#4d555c"

    // -- text ----------------------------------------------------------------
    readonly property color colorText: "#e8e5df"
    readonly property color colorTextMuted: "#98a1a8"
    readonly property color colorTextDisabled: "#5f676e"

    // -- accent (TF2 orange) and decorative team colours ---------------------
    readonly property color colorAccent: "#f0a30a"
    readonly property color colorAccentHover: "#ffb422"
    readonly property color colorAccentPressed: "#c98708"
    readonly property color colorAccentText: "#1a1206"
    readonly property color colorRed: "#b8383b"
    readonly property color colorBlu: "#5885a2"

    // -- key caps ------------------------------------------------------------
    readonly property color colorKeyCap: "#2b3136"
    readonly property color colorKeyCapHover: "#39424a"
    readonly property color colorKeyCapPressed: "#4a545d"
    readonly property color colorKeyCapDisabled: "#1f2427"
    readonly property color colorKeyCapBorder: "#444c53"
    readonly property color colorKeyCapText: "#d7dde2"
    // A bound key. Deliberately muted burnt orange: it has to read as "set" at a glance
    // without competing with Theme.colorAccent, which marks what is interactive (D16).
    readonly property color colorKeyCapBound: "#4e2d17"
    readonly property color colorKeyCapBoundHover: "#5f3920"
    readonly property color colorKeyCapBoundPressed: "#704527"
    readonly property color colorKeyCapBoundBorder: "#9a5a26"

    // -- fonts (families registered from res/font at startup) ----------------
    readonly property string familyBanner: "TF2"
    readonly property string familyHeading: "TF2 Professor"
    readonly property string familyButton: "TF2 Build"
    readonly property string familyUi: "TF2"
    readonly property string familyMono: "Liberation Mono"

    readonly property int sizeBanner: 40
    readonly property int sizeHeading: 26
    readonly property int sizeButton: 13
    readonly property int sizeUi: 13
    readonly property int sizeMono: 12
    readonly property int sizeSmall: 10

    // -- metrics -------------------------------------------------------------
    readonly property int spacingTight: 4
    readonly property int spacing: 8
    readonly property int spacingWide: 16
    readonly property int spacingSection: 24
    readonly property int radius: 4
    readonly property int borderWidth: 1
    readonly property int controlHeight: 32
    readonly property int comboDefaultWidth: 200
    readonly property int popupMaxHeight: 300
    readonly property int tabHeight: 34
    readonly property int tabMinWidth: 140
    readonly property int statusBarHeight: 26
    readonly property int capSize: 64
    readonly property int capGap: 4
    readonly property int capClusterGap: 24
    // One window size for both screens. The start screen is fixed at this size; the editor
// opens at it too and is resizable, with this as its floor.
readonly property int windowWidth: 1000
    readonly property int windowHeight: 500
    readonly property int panelMaxWidth: 480
}
