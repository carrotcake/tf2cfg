#ifndef KEYBFORM_H
#define KEYBFORM_H

#include <QWidget>

namespace Ui {
class KeybForm;
}

class KeybForm : public QWidget {
    Q_OBJECT

public:
    enum KeyId {
        K_ESC = 0,
        K_F1,
        K_F2,
        K_F3,
        K_F4,
        K_F5,
        K_F6,
        K_F7,
        K_F8,
        K_F9,
        K_F10,
        K_F11,
        K_F12,
        K_SCRLK,
        K_PAUSE,
        K_TILDE,
        K_1,
        K_2,
        K_3,
        K_4,
        K_5,
        K_6,
        K_7,
        K_8,
        K_9,
        K_0,
        K_MINUS,
        K_EQ,
        K_BKSPC,
        K_TAB,
        K_Q,
        K_W,
        K_E,
        K_R,
        K_T,
        K_Y,
        K_U,
        K_I,
        K_O,
        K_P,
        K_LB,
        K_RB,
        K_BSLSH,
        K_CAPSLK,
        K_A,
        K_S,
        K_D,
        K_F,
        K_G,
        K_H,
        K_J,
        K_K,
        K_L,
        K_SEMI,
        K_APOS,
        K_ENTER,
        K_LSHFT,
        K_Z,
        K_X,
        K_C,
        K_V,
        K_B,
        K_N,
        K_M,
        K_COMMA,
        K_DOT,
        K_FSLSH,
        K_RSHFT,
        K_LCTRL,
        K_LSUP,
        K_LALT,
        K_SPACE,
        K_RALT,
        K_RSUP,
        K_RCTRL,
        K_INS,
        K_HOME,
        K_PGUP,
        K_DEL,
        K_END,
        K_PGDN,
        K_UP,
        K_LEFT,
        K_DOWN,
        K_RIGHT,
        K_NUMLK,
        K_KPSLSH,
        K_KPSTAR,
        K_KPMINUS,
        K_KP7,
        K_KP8,
        K_KP9,
        K_KP4,
        K_KP5,
        K_KP6,
        K_KP1,
        K_KP2,
        K_KP3,
        K_KP0,
        K_KPDOT,
        K_KPPLUS,
        K_KPENTER,
        KEYCOUNT // not a valid key code
    };
    inline static const QString KEY_CODES[KEYCOUNT] = {"escape",
                                                       "f1",
                                                       "f2",
                                                       "f3",
                                                       "f4",
                                                       "f5",
                                                       "f6",
                                                       "f7",
                                                       "f8",
                                                       "f9",
                                                       "f10",
                                                       "f11",
                                                       "f12",
                                                       "scrollock",
                                                       "pause",
                                                       "`",
                                                       "1",
                                                       "2",
                                                       "3",
                                                       "4",
                                                       "5",
                                                       "6",
                                                       "7",
                                                       "8",
                                                       "9",
                                                       "0",
                                                       "-",
                                                       "=",
                                                       "backspace",
                                                       "tab",
                                                       "q",
                                                       "w",
                                                       "e",
                                                       "r",
                                                       "t",
                                                       "y",
                                                       "u",
                                                       "i",
                                                       "o",
                                                       "p",
                                                       "[",
                                                       "]",
                                                       "\\",
                                                       "capslock",
                                                       "a",
                                                       "s",
                                                       "d",
                                                       "f",
                                                       "g",
                                                       "h",
                                                       "j",
                                                       "k",
                                                       "l",
                                                       ";",
                                                       "'",
                                                       "enter",
                                                       "shift",
                                                       "z",
                                                       "x",
                                                       "c",
                                                       "v",
                                                       "b",
                                                       "n",
                                                       "m",
                                                       ",",
                                                       ".",
                                                       "/",
                                                       "rshift",
                                                       "ctrl",
                                                       "lwin",
                                                       "alt",
                                                       "space",
                                                       "ralt",
                                                       "rwin",
                                                       "rctrl",
                                                       "ins",
                                                       "home",
                                                       "pgup",
                                                       "del",
                                                       "end",
                                                       "pgdn",
                                                       "uparrow",
                                                       "leftarrow",
                                                       "downarrow",
                                                       "rightarrow",
                                                       "numlock",
                                                       "kp_slash",
                                                       "kp_multiply",
                                                       "kp_minus",
                                                       "KP_home",
                                                       "kp_uparrow",
                                                       "kp_pgup",
                                                       "kp_leftarrow",
                                                       "kp_5",
                                                       "kp_rightarrow",
                                                       "kp_end",
                                                       "kp_downarrow",
                                                       "kp_pgdn",
                                                       "kp_ins",
                                                       "kp_del",
                                                       "kp_plus",
                                                       "kp_enter"};

    explicit KeybForm(QWidget *parent = nullptr);
    ~KeybForm();

signals:
    void keybButtonClicked(int id);

private slots:
    void on_keybButtons_idClicked(int id);

private:
    Ui::KeybForm *ui;
    void setupButtonGroup();
};

#endif // KEYBFORM_H
