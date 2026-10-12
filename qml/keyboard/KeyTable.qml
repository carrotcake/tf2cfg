pragma Singleton

import QtQuick
import "../theme"

// The key table: one entry per physical cap, plus the row layout used to draw the
// keyboard. See docs/ui-qml-rewrite.md sections 7 and 8.
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
        "kpenter":    { id: 101, token: "kp_enter",      label: "KP Enter", rows: 2 }
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

    // -- queries ------------------------------------------------------------

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
