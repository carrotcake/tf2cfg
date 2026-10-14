#include "keytable.h"

#include <QVariant>

KeyTable::KeyTable(QObject *parent) : QObject(parent) {
    // -- function row ---------------------------------------------------------
    add("esc", 0, "escape", "Esc");
    add("f1", 1, "f1", "F1");
    add("f2", 2, "f2", "F2");
    add("f3", 3, "f3", "F3");
    add("f4", 4, "f4", "F4");
    add("f5", 5, "f5", "F5");
    add("f6", 6, "f6", "F6");
    add("f7", 7, "f7", "F7");
    add("f8", 8, "f8", "F8");
    add("f9", 9, "f9", "F9");
    add("f10", 10, "f10", "F10");
    add("f11", 11, "f11", "F11");
    add("f12", 12, "f12", "F12");
    add("prtsc", -1, "", "PrtScn", 0, 0, 0, "This key cannot be bound to an action.");
    add("scrlk", 13, "scrollock", "ScrLk");
    add("pause", 14, "pause", "Pause");

    // -- number row -----------------------------------------------------------
    add("tilde", 15, "`", "`");
    add("k1", 16, "1", "1");
    add("k2", 17, "2", "2");
    add("k3", 18, "3", "3");
    add("k4", 19, "4", "4");
    add("k5", 20, "5", "5");
    add("k6", 21, "6", "6");
    add("k7", 22, "7", "7");
    add("k8", 23, "8", "8");
    add("k9", 24, "9", "9");
    add("k0", 25, "0", "0");
    add("minus", 26, "-", "-");
    add("eq", 27, "=", "=");
    add("bkspc", 28, "backspace", "Backspace", 128);

    // -- QWERTY row -----------------------------------------------------------
    add("tab", 29, "tab", "Tab", 96);
    add("q", 30, "q", "Q");
    add("w", 31, "w", "W");
    add("e", 32, "e", "E");
    add("r", 33, "r", "R");
    add("t", 34, "t", "T");
    add("y", 35, "y", "Y");
    add("u", 36, "u", "U");
    add("i", 37, "i", "I");
    add("o", 38, "o", "O");
    add("p", 39, "p", "P");
    add("lb", 40, "[", "[");
    add("rb", 41, "]", "]");
    add("bslsh", 42, "\\", "\\", 96);

    // -- home row -------------------------------------------------------------
    add("capslk", 43, "capslock", "Caps Lock", 116);
    add("a", 44, "a", "A");
    add("s", 45, "s", "S");
    add("d", 46, "d", "D");
    add("f", 47, "f", "F");
    add("g", 48, "g", "G");
    add("h", 49, "h", "H");
    add("j", 50, "j", "J");
    add("k", 51, "k", "K");
    add("l", 52, "l", "L");
    add("semi", 53, ";", ";");
    add("apos", 54, "'", "'");
    add("enter", 55, "enter", "Enter", 144);

    // -- bottom rows of the main block ----------------------------------------
    add("lshift", 56, "shift", "Left Shift", 148);
    add("z", 57, "z", "Z");
    add("x", 58, "x", "X");
    add("c", 59, "c", "C");
    add("v", 60, "v", "V");
    add("b", 61, "b", "B");
    add("n", 62, "n", "N");
    add("m", 63, "m", "M");
    add("comma", 64, ",", ",");
    add("dot", 65, ".", ".");
    add("fslsh", 66, "/", "/");
    add("rshift", 67, "rshift", "Right Shift", 176);
    add("lctrl", 68, "ctrl", "Left Ctrl", 80);
    add("lsup", 69, "lwin", "Left Win", 80);
    add("lalt", 70, "alt", "Left Alt", 80);
    add("space", 71, "space", "Space", 416);
    add("ralt", 72, "ralt", "Right Alt", 80);
    add("rsup", 73, "rwin", "Right Win", 80);
    add("menu", -1, "", "Menu", 80, 0, 0, "This key cannot be bound to an action.");
    add("rctrl", 74, "rctrl", "Right Ctrl", 80);

    // -- middle cluster -------------------------------------------------------
    add("ins", 75, "ins", "Ins");
    add("home", 76, "home", "Home");
    add("pgup", 77, "pgup", "PgUp");
    add("del", 78, "del", "Del");
    add("end", 79, "end", "End");
    add("pgdn", 80, "pgdn", "PgDn");
    add("uparrow", 81, "uparrow", "Up");
    add("leftarrow", 82, "leftarrow", "Left");
    add("downarrow", 83, "downarrow", "Down");
    add("rightarrow", 84, "rightarrow", "Right");

    // -- numpad ---------------------------------------------------------------
    add("numlk", 85, "numlock", "Num Lock");
    add("kpslsh", 86, "kp_slash", "KP /");
    add("kpmult", 87, "kp_multiply", "KP *");
    add("kpminus", 88, "kp_minus", "KP -");
    add("kp7", 89, "KP_home", "KP 7");
    add("kp8", 90, "kp_uparrow", "KP 8");
    add("kp9", 91, "kp_pgup", "KP 9");
    add("kp4", 92, "kp_leftarrow", "KP 4");
    add("kp5", 93, "kp_5", "KP 5");
    add("kp6", 94, "kp_rightarrow", "KP 6");
    add("kp1", 95, "kp_end", "KP 1");
    add("kp2", 96, "kp_downarrow", "KP 2");
    add("kp3", 97, "kp_pgdn", "KP 3");
    add("kp0", 98, "kp_ins", "KP 0", 128);
    add("kpdot", 99, "kp_del", "KP .");
    add("kpplus", 100, "kp_plus", "KP +", 0, 2);
    add("kpenter", 101, "kp_enter", "KP Enter", 0, 2);

    // -- mouse buttons and wheel ----------------------------------------------
    // Not in the widget build's keybform.h, so the ids continue after the
    // keyboard's 0..101. Tokens are the engine's own mouse input names.
    // Labels run long, so the caps take the smaller fs override.
    add("mouse1", 102, "mouse1", "Left Click", 0, 0, 10);
    add("mouse2", 103, "mouse2", "Right Click", 0, 0, 10);
    add("mouse3", 104, "mouse3", "Middle Click", 0, 0, 10);
    add("mouse4", 105, "mouse4", "Back", 0, 0, 10);
    add("mouse5", 106, "mouse5", "Forward", 0, 0, 10);
    add("mwheelup", 107, "mwheelup", "Scroll Up", 0, 0, 10);
    add("mwheeldown", 108, "mwheeldown", "Scroll Down", 0, 0, 10);

    // -- controller (Xbox 360 style, see docs/legacy/missing-input-ids.md) ----
    // Buttons are the engine's JOY codes; triggers/sticks/d-pad are the
    // axis-direction and POV keys. Axis tokens contain spaces - they must stay
    // quoted in cfg output, which CommandListModel already handles.
    add("joy1", 109, "joy1", "A");
    add("joy2", 110, "joy2", "B");
    add("joy3", 111, "joy3", "X");
    add("joy4", 112, "joy4", "Y");
    add("joy5", 113, "joy5", "L Bumper");
    add("joy6", 114, "joy6", "R Bumper");
    add("joy7", 115, "joy7", "Back");
    add("joy8", 116, "joy8", "Start");
    add("joy9", 117, "joy9", "L3");
    add("joy10", 118, "joy10", "R3");
    add("pov_up", 119, "pov_up", "D UP", 0, 0, 10);
    add("pov_right", 120, "pov_right", "D R", 0, 0, 10);
    add("pov_down", 121, "pov_down", "D DN", 0, 0, 10);
    add("pov_left", 122, "pov_left", "D L", 0, 0, 10);
    add("x axis pos", 123, "x axis pos", "LS R", 0, 0, 10);
    add("x axis neg", 124, "x axis neg", "LS L", 0, 0, 10);
    add("y axis pos", 125, "y axis pos", "LS DN", 0, 0, 10);
    add("y axis neg", 126, "y axis neg", "LS UP", 0, 0, 10);
    add("u axis pos", 127, "u axis pos", "RS R", 0, 0, 10);
    add("u axis neg", 128, "u axis neg", "RS L", 0, 0, 10);
    add("v axis pos", 129, "v axis pos", "RS DN", 0, 0, 10);
    add("v axis neg", 130, "v axis neg", "RS UP", 0, 0, 10);
    add("z axis pos", 131, "z axis pos", "LT");
    add("z axis neg", 132, "z axis neg", "RT");

    // -- layout rows (keybform.ui parity) --------------------------------------
    m_fnRowGroups = {QVariantList{"esc"}, QVariantList{"f1", "f2", "f3", "f4"},
                     QVariantList{"f5", "f6", "f7", "f8"},
                     QVariantList{"f9", "f10", "f11", "f12"},
                     QVariantList{"prtsc", "scrlk", "pause"}};
    m_mainRows = {QVariantList{"tilde", "k1", "k2", "k3", "k4", "k5", "k6", "k7", "k8",
                                "k9", "k0", "minus", "eq", "bkspc"},
                  QVariantList{"tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
                               "lb", "rb", "bslsh"},
                  QVariantList{"capslk", "a", "s", "d", "f", "g", "h", "j", "k", "l",
                               "semi", "apos", "enter"},
                  QVariantList{"lshift", "z", "x", "c", "v", "b", "n", "m", "comma",
                               "dot", "fslsh", "rshift"},
                  QVariantList{"lctrl", "lsup", "lalt", "space", "ralt", "rsup", "menu",
                               "rctrl"}};
    // A null entry is the blank slot above the arrow keys.
    QVariantList middleBlank;
    m_middleRows = {QVariantList{"ins", "home", "pgup"}, QVariantList{"del", "end", "pgdn"},
                    middleBlank, QVariantList{"uparrow"},
                    QVariantList{"leftarrow", "downarrow", "rightarrow"}};
    m_numpadMain = {QVariantList{"numlk", "kpslsh", "kpmult"}, QVariantList{"kp7", "kp8", "kp9"},
                    QVariantList{"kp4", "kp5", "kp6"}, QVariantList{"kp1", "kp2", "kp3"},
                    QVariantList{"kp0", "kpdot"}};
    m_numpadRight = {QVariantList{"kpminus"}, QVariantList{"kpplus"}, QVariantList{"kpenter"}};

    // -- mouse block: a mouse seen from above, rotated 90 degrees counter-clockwise
    // so the front edge is the LEFT side. Absolute cell geometry (px), sized like
    // the real thing: front edge = split main button (Right above, Left below);
    // the wheel column with the press flanked by the scrolls; side buttons along
    // the bottom edge - Forward then Back, Back rightmost. Both anchors hold:
    // top-left = Right Click, bottom-right = Back.
    const auto cell = [](const char *name, int x, int y, int w, int h) {
        QVariantMap m;
        m.insert("name", name);
        m.insert("x", x);
        m.insert("y", y);
        m.insert("w", w);
        m.insert("h", h);
        return m;
    };
    m_mouseCells = {cell("mouse2", 0, 0, 96, 90),      cell("mouse1", 0, 94, 96, 90),
                    cell("mwheelup", 100, 0, 112, 56), cell("mouse3", 100, 60, 112, 64),
                    cell("mwheeldown", 100, 128, 112, 56), cell("mouse5", 13, 188, 91, 48),
                    cell("mouse4", 108, 188, 91, 48)};
    m_mouseWidth = blockExtent(m_mouseCells, "x", "w");
    m_mouseHeight = blockExtent(m_mouseCells, "y", "h");

    // -- controller block: an Xbox 360 pad seen from above, trigger/bumper edge
    // at the TOP, mirror-symmetric around x = 235; matching pairs share size.
    // Axis tokens spell the pressed direction per the SDK: X/U = horizontal,
    // Y/V = vertical (negative = up), Z negative = right trigger.
    m_controllerCells = {
        cell("z axis pos", 130, 0, 80, 36),    cell("z axis neg", 260, 0, 80, 36),
        cell("joy5", 25, 40, 110, 40),         cell("joy6", 335, 40, 110, 40),
        cell("y axis neg", 86, 94, 48, 40),    cell("x axis neg", 38, 136, 48, 40),
        cell("joy9", 94, 140, 32, 32),         cell("x axis pos", 134, 136, 48, 40),
        cell("y axis pos", 86, 178, 48, 40),
        cell("joy7", 138, 71, 88, 32),         cell("joy8", 242, 71, 88, 32),
        cell("joy4", 336, 94, 48, 40),         cell("joy3", 288, 136, 48, 40),
        cell("joy1", 336, 178, 48, 40),        cell("joy2", 384, 136, 48, 40),
        cell("pov_up", 102, 228, 48, 40),      cell("pov_left", 54, 270, 48, 40),
        cell("pov_right", 150, 270, 48, 40),   cell("pov_down", 102, 312, 48, 40),
        cell("v axis neg", 320, 228, 48, 40),  cell("u axis neg", 272, 270, 48, 40),
        cell("joy10", 328, 274, 32, 32),       cell("u axis pos", 368, 270, 48, 40),
        cell("v axis pos", 320, 312, 48, 40)};
    m_controllerWidth = blockExtent(m_controllerCells, "x", "w");
    m_controllerHeight = blockExtent(m_controllerCells, "y", "h");
}

void KeyTable::add(const QString &name, int id, const QString &token, const QString &label,
                   qreal w, int rows, int fs, const QString &tooltip) {
    // Un-bindable caps are exactly the ones with no engine id and no token
    // (prtsc, menu); every real cap has both.
    m_caps.insert(name, Cap{id, token, label, w, rows, fs,
                            id >= 0 && !token.isEmpty(), tooltip});
}

qreal KeyTable::blockExtent(const QVariantList &cells, const char *start, const char *span) {
    qreal m = 0;
    for (const QVariant &v : cells) {
        const QVariantMap c = v.toMap();
        const qreal extent = c.value(start).toDouble() + c.value(span).toDouble();
        if (extent > m)
            m = extent;
    }
    return m;
}

QVariantMap KeyTable::keyData(const QString &name) const {
    const auto it = m_caps.constFind(name);
    if (it == m_caps.constEnd())
        return {};
    QVariantMap m;
    m.insert("id", it->id);
    m.insert("token", it->token);
    m.insert("label", it->label);
    if (it->w > 0)
        m.insert("w", it->w);
    if (it->rows > 0)
        m.insert("rows", it->rows);
    if (it->fs > 0)
        m.insert("fs", it->fs);
    if (!it->bindable)
        m.insert("bindable", false);
    if (!it->tooltip.isEmpty())
        m.insert("tooltip", it->tooltip);
    return m;
}

bool KeyTable::isBindable(const QString &name) const {
    const auto it = m_caps.constFind(name);
    return it != m_caps.constEnd() && it->bindable;
}

qreal KeyTable::keyWidth(const QString &name, qreal defaultW) const {
    const auto it = m_caps.constFind(name);
    return it == m_caps.constEnd() || it->w <= 0 ? defaultW : it->w;
}

qreal KeyTable::keyHeight(const QString &name, qreal capSize, qreal capGap) const {
    const auto it = m_caps.constFind(name);
    const int rows = it == m_caps.constEnd() || it->rows <= 0 ? 1 : it->rows;
    return rows * capSize + (rows - 1) * capGap;
}

QString KeyTable::keyLabel(const QString &name) const {
    const auto it = m_caps.constFind(name);
    return it == m_caps.constEnd() ? name : it->label;
}

QString KeyTable::keyToken(const QString &name) const {
    const auto it = m_caps.constFind(name);
    return it == m_caps.constEnd() ? QString() : it->token;
}

int KeyTable::keyId(const QString &name) const {
    const auto it = m_caps.constFind(name);
    return it == m_caps.constEnd() ? -1 : it->id;
}

QString KeyTable::tokenForId(int id) const {
    for (auto it = m_caps.constBegin(); it != m_caps.constEnd(); ++it) {
        if (it->id == id)
            return it->token;
    }
    return QString();
}

QString KeyTable::nameForToken(const QString &token) const {
    for (auto it = m_caps.constBegin(); it != m_caps.constEnd(); ++it) {
        if (it->token == token)
            return it.key();
    }
    return QString();
}