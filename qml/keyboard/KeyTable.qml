pragma Singleton

import QtQuick
import "../theme"

// The key table: one entry per physical cap, plus the row layout used to draw the
// keyboard. See docs/legacy/ui-qml-rewrite.md sections 7 and 8.
//
// `id` and `token` are a parity contract with the retired widget build
// (keybform.h KeyId/KEYCODES). Do not renumber and do not "fix" token spellings -
// they are the engine's own strings and a future bind engine depends on both.
QtObject {
    // name -> { id, token, label, w?, rows?, bindable?, tooltip? }
    //   w     : cap width in px (default Theme.capSize = 64)
    //   rows  : how many cap rows the key spans (default 1) - numpad + and Enter only
    readonly property var keys: ({
        // -- function row ----------------------------------------------------
        "esc":        { id: 0,   token: "escape",     label: "Esc" },
        "f1":         { id: 1,   token: "f1",         label: "F1" },
        "f2":         { id: 2,   token: "f2",         label: "F2" },
        "f3":         { id: 3,   token: "f3",         label: "F3" },
        "f4":         { id: 4,   token: "f4",         label: "F4" },
        "f5":         { id: 5,   token: "f5",         label: "F5" },
        "f6":         { id: 6,   token: "f6",         label: "F6" },
        "f7":         { id: 7,   token: "f7",         label: "F7" },
        "f8":         { id: 8,   token: "f8",         label: "F8" },
        "f9":         { id: 9,   token: "f9",         label: "F9" },
        "f10":        { id: 10,  token: "f10",        label: "F10" },
        "f11":        { id: 11,  token: "f11",        label: "F11" },
        "f12":        { id: 12,  token: "f12",        label: "F12" },
        "prtsc":      { id: -1,  token: "",           label: "PrtScn", bindable: false,
                        tooltip: "This key cannot be bound to an action." },
        "scrlk":      { id: 13,  token: "scrollock",  label: "ScrLk" },
        "pause":      { id: 14,  token: "pause",      label: "Pause" },

        // -- number row ------------------------------------------------------
        "tilde":      { id: 15,  token: "`",          label: "`" },
        "k1":         { id: 16,  token: "1",          label: "1" },
        "k2":         { id: 17,  token: "2",          label: "2" },
        "k3":         { id: 18,  token: "3",          label: "3" },
        "k4":         { id: 19,  token: "4",          label: "4" },
        "k5":         { id: 20,  token: "5",          label: "5" },
        "k6":         { id: 21,  token: "6",          label: "6" },
        "k7":         { id: 22,  token: "7",          label: "7" },
        "k8":         { id: 23,  token: "8",          label: "8" },
        "k9":         { id: 24,  token: "9",          label: "9" },
        "k0":         { id: 25,  token: "0",          label: "0" },
        "minus":      { id: 26,  token: "-",          label: "-" },
        "eq":         { id: 27,  token: "=",          label: "=" },
        "bkspc":      { id: 28,  token: "backspace",  label: "Backspace", w: 128 },

        // -- QWERTY row ------------------------------------------------------
        "tab":        { id: 29,  token: "tab",        label: "Tab", w: 96 },
        "q":          { id: 30,  token: "q",          label: "Q" },
        "w":          { id: 31,  token: "w",          label: "W" },
        "e":          { id: 32,  token: "e",          label: "E" },
        "r":          { id: 33,  token: "r",          label: "R" },
        "t":          { id: 34,  token: "t",          label: "T" },
        "y":          { id: 35,  token: "y",          label: "Y" },
        "u":          { id: 36,  token: "u",          label: "U" },
        "i":          { id: 37,  token: "i",          label: "I" },
        "o":          { id: 38,  token: "o",          label: "O" },
        "p":          { id: 39,  token: "p",          label: "P" },
        "lb":         { id: 40,  token: "[",          label: "[" },
        "rb":         { id: 41,  token: "]",          label: "]" },
        "bslsh":      { id: 42,  token: "\\",         label: "\\", w: 96 },

        // -- home row --------------------------------------------------------
        "capslk":     { id: 43,  token: "capslock",   label: "Caps Lock", w: 116 },
        "a":          { id: 44,  token: "a",          label: "A" },
        "s":          { id: 45,  token: "s",          label: "S" },
        "d":          { id: 46,  token: "d",          label: "D" },
        "f":          { id: 47,  token: "f",          label: "F" },
        "g":          { id: 48,  token: "g",          label: "G" },
        "h":          { id: 49,  token: "h",          label: "H" },
        "j":          { id: 50,  token: "j",          label: "J" },
        "k":          { id: 51,  token: "k",          label: "K" },
        "l":          { id: 52,  token: "l",          label: "L" },
        "semi":       { id: 53,  token: ";",          label: ";" },
        "apos":       { id: 54,  token: "'",          label: "'" },
        "enter":      { id: 55,  token: "enter",      label: "Enter", w: 144 },

        // -- bottom rows of the main block -----------------------------------
        "lshift":     { id: 56,  token: "shift",      label: "Left Shift", w: 148 },
        "z":          { id: 57,  token: "z",          label: "Z" },
        "x":          { id: 58,  token: "x",          label: "X" },
        "c":          { id: 59,  token: "c",          label: "C" },
        "v":          { id: 60,  token: "v",          label: "V" },
        "b":          { id: 61,  token: "b",          label: "B" },
        "n":          { id: 62,  token: "n",          label: "N" },
        "m":          { id: 63,  token: "m",          label: "M" },
        "comma":      { id: 64,  token: ",",          label: "," },
        "dot":        { id: 65,  token: ".",          label: "." },
        "fslsh":      { id: 66,  token: "/",          label: "/" },
        "rshift":     { id: 67,  token: "rshift",     label: "Right Shift", w: 176 },
        "lctrl":      { id: 68,  token: "ctrl",       label: "Left Ctrl", w: 80 },
        "lsup":       { id: 69,  token: "lwin",       label: "Left Win", w: 80 },
        "lalt":       { id: 70,  token: "alt",        label: "Left Alt", w: 80 },
        "space":      { id: 71,  token: "space",      label: "Space", w: 416 },
        "ralt":       { id: 72,  token: "ralt",       label: "Right Alt", w: 80 },
        "rsup":       { id: 73,  token: "rwin",       label: "Right Win", w: 80 },
        "menu":       { id: -1,  token: "",           label: "Menu", w: 80, bindable: false,
                        tooltip: "This key cannot be bound to an action." },
        "rctrl":      { id: 74,  token: "rctrl",      label: "Right Ctrl", w: 80 },

        // -- middle cluster ---------------------------------------------------
        "ins":        { id: 75,  token: "ins",        label: "Ins" },
        "home":       { id: 76,  token: "home",       label: "Home" },
        "pgup":       { id: 77,  token: "pgup",       label: "PgUp" },
        "del":        { id: 78,  token: "del",        label: "Del" },
        "end":        { id: 79,  token: "end",        label: "End" },
        "pgdn":       { id: 80,  token: "pgdn",       label: "PgDn" },
        "uparrow":    { id: 81,  token: "uparrow",    label: "Up" },
        "leftarrow":  { id: 82,  token: "leftarrow",  label: "Left" },
        "downarrow":  { id: 83,  token: "downarrow",  label: "Down" },
        "rightarrow": { id: 84,  token: "rightarrow", label: "Right" },

        // -- numpad -----------------------------------------------------------
        "numlk":      { id: 85,  token: "numlock",       label: "Num Lock" },
        "kpslsh":     { id: 86,  token: "kp_slash",      label: "KP /" },
        "kpmult":     { id: 87,  token: "kp_multiply",   label: "KP *" },
        "kpminus":    { id: 88,  token: "kp_minus",      label: "KP -" },
        "kp7":        { id: 89,  token: "KP_home",       label: "KP 7" },
        "kp8":        { id: 90,  token: "kp_uparrow",    label: "KP 8" },
        "kp9":        { id: 91,  token: "kp_pgup",       label: "KP 9" },
        "kp4":        { id: 92,  token: "kp_leftarrow",  label: "KP 4" },
        "kp5":        { id: 93,  token: "kp_5",          label: "KP 5" },
        "kp6":        { id: 94,  token: "kp_rightarrow", label: "KP 6" },
        "kp1":        { id: 95,  token: "kp_end",        label: "KP 1" },
        "kp2":        { id: 96,  token: "kp_downarrow",  label: "KP 2" },
        "kp3":        { id: 97,  token: "kp_pgdn",       label: "KP 3" },
        "kp0":        { id: 98,  token: "kp_ins",        label: "KP 0", w: 128 },
        "kpdot":      { id: 99,  token: "kp_del",        label: "KP ." },
        "kpplus":     { id: 100, token: "kp_plus",       label: "KP +", rows: 2 },
        "kpenter":    { id: 101, token: "kp_enter",      label: "KP Enter", rows: 2 },

        // -- mouse buttons and wheel ------------------------------------------
        // Not in the widget build's keybform.h, so the ids continue after the
        // keyboard's 0..101. Tokens are the engine's own mouse input names.
        // Labels run long, so the caps take the smaller fs override.
        "mouse1":     { id: 102, token: "mouse1",        label: "Left Click", fs: 10 },
        "mouse2":     { id: 103, token: "mouse2",        label: "Right Click", fs: 10 },
        "mouse3":     { id: 104, token: "mouse3",        label: "Middle Click", fs: 10 },
        "mouse4":     { id: 105, token: "mouse4",        label: "Back", fs: 10 },
        "mouse5":     { id: 106, token: "mouse5",        label: "Forward", fs: 10 },
        "mwheelup":   { id: 107, token: "mwheelup",      label: "Scroll Up", fs: 10 },
        "mwheeldown": { id: 108, token: "mwheeldown",    label: "Scroll Down", fs: 10 },

        // -- controller (Xbox 360 style, see docs/missing-input-ids.md) --------
        // Buttons are the engine's JOY codes; triggers/sticks/d-pad are the
        // axis-direction and POV keys. Not in keybform.h, so ids continue on.
        // Axis tokens contain spaces - they must stay quoted in cfg output,
        // which CommandListModel already handles.
        "joy1":       { id: 109, token: "joy1",          label: "A" },
        "joy2":       { id: 110, token: "joy2",          label: "B" },
        "joy3":       { id: 111, token: "joy3",          label: "X" },
        "joy4":       { id: 112, token: "joy4",          label: "Y" },
        "joy5":       { id: 113, token: "joy5",          label: "L Bumper" },
        "joy6":       { id: 114, token: "joy6",          label: "R Bumper" },
        "joy7":       { id: 115, token: "joy7",          label: "Back" },
        "joy8":       { id: 116, token: "joy8",          label: "Start" },
        "joy9":       { id: 117, token: "joy9",          label: "L3" },
        "joy10":      { id: 118, token: "joy10",         label: "R3" },
        "pov_up":     { id: 119, token: "pov_up",        label: "D UP", fs: 10 },
        "pov_right":  { id: 120, token: "pov_right",     label: "D R", fs: 10 },
        "pov_down":   { id: 121, token: "pov_down",      label: "D DN", fs: 10 },
        "pov_left":   { id: 122, token: "pov_left",      label: "D L", fs: 10 },
        "x axis pos": { id: 123, token: "x axis pos",    label: "LS R", fs: 10 },
        "x axis neg": { id: 124, token: "x axis neg",    label: "LS L", fs: 10 },
        "y axis pos": { id: 125, token: "y axis pos",    label: "LS DN", fs: 10 },
        "y axis neg": { id: 126, token: "y axis neg",    label: "LS UP", fs: 10 },
        "u axis pos": { id: 127, token: "u axis pos",    label: "RS R", fs: 10 },
        "u axis neg": { id: 128, token: "u axis neg",    label: "RS L", fs: 10 },
        "v axis pos": { id: 129, token: "v axis pos",    label: "RS DN", fs: 10 },
        "v axis neg": { id: 130, token: "v axis neg",    label: "RS UP", fs: 10 },
        "z axis pos": { id: 131, token: "z axis pos",    label: "LT" },
        "z axis neg": { id: 132, token: "z axis neg",    label: "RT" }
    })

    // Layout, mirroring the widget build's frames (keybform.ui).
    // A null entry in `middleRows` is the blank slot above the arrow keys.
    readonly property var fnRowGroups: [
        ["esc"],
        ["f1", "f2", "f3", "f4"],
        ["f5", "f6", "f7", "f8"],
        ["f9", "f10", "f11", "f12"],
        ["prtsc", "scrlk", "pause"]
    ]
    readonly property var mainRows: [
        ["tilde", "k1", "k2", "k3", "k4", "k5", "k6", "k7", "k8", "k9", "k0", "minus", "eq", "bkspc"],
        ["tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "lb", "rb", "bslsh"],
        ["capslk", "a", "s", "d", "f", "g", "h", "j", "k", "l", "semi", "apos", "enter"],
        ["lshift", "z", "x", "c", "v", "b", "n", "m", "comma", "dot", "fslsh", "rshift"],
        ["lctrl", "lsup", "lalt", "space", "ralt", "rsup", "menu", "rctrl"]
    ]
    readonly property var middleRows: [
        ["ins", "home", "pgup"],
        ["del", "end", "pgdn"],
        null,
        ["uparrow"],
        ["leftarrow", "downarrow", "rightarrow"]
    ]
    readonly property var numpadMain: [
        ["numlk", "kpslsh", "kpmult"],
        ["kp7", "kp8", "kp9"],
        ["kp4", "kp5", "kp6"],
        ["kp1", "kp2", "kp3"],
        ["kp0", "kpdot"]
    ]
    readonly property var numpadRight: ["kpminus", "kpplus", "kpenter"]

    // Mouse buttons/wheel, drawn below the keyboard: a mouse seen from above,
    // rotated 90 degrees counter-clockwise so the front edge is the LEFT side.
    // Absolute cell geometry (x/y/w/h in px), sized like the real thing:
    //   - the front edge is the split main button - Right Click above, Left
    //     Click below, identical halves;
    //   - the wheel column: Scroll Up, Middle Click (the press), Scroll Down,
    //     the two scrolls identical and flanking the press;
    //   - the side buttons along the bottom edge - Forward then Back, identical,
    //     Back rightmost.
    // Both anchors hold: top-left = Right Click, bottom-right = Back.
    readonly property var mouseCells: [
        { name: "mouse2",     x: 0,   y: 0,   w: 96, h: 90 },
        { name: "mouse1",     x: 0,   y: 94,  w: 96, h: 90 },
        { name: "mwheelup",   x: 100, y: 0,   w: 112, h: 56 },
        { name: "mouse3",     x: 100, y: 60,  w: 112, h: 64 },
        { name: "mwheeldown", x: 100, y: 128, w: 112, h: 56 },
        { name: "mouse5",     x: 13,  y: 188, w: 91, h: 48 },
        { name: "mouse4",     x: 108, y: 188, w: 91, h: 48 }
    ]
    readonly property real mouseWidth: blockExtent(mouseCells, "x", "w")
    readonly property real mouseHeight: blockExtent(mouseCells, "y", "h")

    // Controller buttons/wheel block: an Xbox 360 pad seen from above with the
    // trigger/bumper edge at the TOP. Absolute cell geometry (px), mirror-
    // symmetric around x = 235; matching pairs share size:
    //   - LT/RT and LB/RB along the top edge;
    //   - left stick: four direction keys in a ring, L3 (joy9) as its center;
    //   - ABXY diamond on the upper right, mirroring the stick ring;
    //   - Back/Start pair centered between the halves;
    //   - D-pad as a four-way plus on the lower left;
    //   - right stick ring with R3 (joy10) as its center, mirroring the D-pad.
    // Axis tokens spell the pressed direction per the SDK: X/U = horizontal,
    // Y/V = vertical (negative = up), Z negative = right trigger.
    readonly property var controllerCells: [
        { name: "z axis pos", x: 130, y: 0,   w: 80,  h: 36 },  // LT
        { name: "z axis neg", x: 260, y: 0,   w: 80,  h: 36 },  // RT
        { name: "joy5",       x: 25,  y: 40,  w: 110, h: 40 },
        { name: "joy6",       x: 335, y: 40,  w: 110, h: 40 },

        { name: "y axis neg", x: 86,  y: 94,  w: 48, h: 40 },   // LS UP
        { name: "x axis neg", x: 38,  y: 136, w: 48, h: 40 },   // LS L
        { name: "joy9",       x: 94,  y: 140, w: 32, h: 32 },   // L3
        { name: "x axis pos", x: 134, y: 136, w: 48, h: 40 },   // LS R
        { name: "y axis pos", x: 86,  y: 178, w: 48, h: 40 },   // LS DN

        // Back/Start sit centered at y 87: halfway between the R bumper and Y
        // rows vertically (bumper center 60, Y center 114).
        { name: "joy7",       x: 138, y: 71,  w: 88, h: 32 },  // Back
        { name: "joy8",       x: 242, y: 71,  w: 88, h: 32 },  // Start

        { name: "joy4",       x: 336, y: 94,  w: 48, h: 40 },  // Y
        { name: "joy3",       x: 288, y: 136, w: 48, h: 40 },  // X
        { name: "joy1",       x: 336, y: 178, w: 48, h: 40 },  // A
        { name: "joy2",       x: 384, y: 136, w: 48, h: 40 },  // B

        // D-pad and right stick sit 16px closer to the center line than the
        // clusters above them, the way the real pad staggers them.
        { name: "pov_up",     x: 102, y: 228, w: 48, h: 40 },
        { name: "pov_left",   x: 54,  y: 270, w: 48, h: 40 },
        { name: "pov_right",  x: 150, y: 270, w: 48, h: 40 },
        { name: "pov_down",   x: 102, y: 312, w: 48, h: 40 },

        { name: "v axis neg", x: 320, y: 228, w: 48, h: 40 },   // RS UP
        { name: "u axis neg", x: 272, y: 270, w: 48, h: 40 },   // RS L
        { name: "joy10",      x: 328, y: 274, w: 32, h: 32 },   // R3
        { name: "u axis pos", x: 368, y: 270, w: 48, h: 40 },   // RS R
        { name: "v axis pos", x: 320, y: 312, w: 48, h: 40 }    // RS DN
    ]
    readonly property real controllerWidth: blockExtent(controllerCells, "x", "w")
    readonly property real controllerHeight: blockExtent(controllerCells, "y", "h")

    // -- queries ------------------------------------------------------------

    /// Right/bottom extent of an absolutely positioned cap block.
    function blockExtent(cells, first, second) {
        let m = 0
        for (const c of cells) {
            const v = c[first] + c[second]
            if (v > m)
                m = v
        }
        return m
    }

    /// Layout data for a cap, or null.
    function keyData(name) {
        return keys[name] !== undefined ? keys[name] : null
    }

    /// Whether a cap can be bound to an action.
    function isBindable(name) {
        const d = keyData(name)
        return d !== null && d.bindable !== false
    }

    /// Cap width in px.
    function keyWidth(name) {
        const d = keyData(name)
        return (d !== null && d.w !== undefined) ? d.w : Theme.capSize
    }

    /// Cap height in px, honouring multi-row keys.
    function keyHeight(name) {
        const d = keyData(name)
        const rows = (d !== null && d.rows !== undefined) ? d.rows : 1
        return rows * Theme.capSize + (rows - 1) * Theme.capGap
    }

    /// Display label of a cap.
    function keyLabel(name) {
        const d = keyData(name)
        return d !== null ? d.label : name
    }

    /// TF2 key token, or empty for the un-bindable caps.
    function keyToken(name) {
        const d = keyData(name)
        return d !== null ? d.token : ""
    }

    /// Engine key id, or -1 for the un-bindable caps.
    function keyId(name) {
        const d = keyData(name)
        return d !== null ? d.id : -1
    }

    /// Reverse lookups, for a future bind engine / keyboard overlay.
    function tokenForId(id) {
        for (const name in keys) {
            if (keys[name].id === id)
                return keys[name].token
        }
        return ""
    }
    function nameForToken(token) {
        for (const name in keys) {
            if (keys[name].token === token)
                return name
        }
        return ""
    }
}
