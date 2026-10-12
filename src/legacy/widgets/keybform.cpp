#include "ui/keybform.h"
#include "ui_keybform.h"

KeybForm::KeybForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::KeybForm) {
    ui->setupUi(this);
    setupButtonGroup();
}

void KeybForm::on_keybButtons_idClicked(int id) {
    if (id > -1 && id < KEYCOUNT) {
        emit keybButtonClicked(id);
    }
}

KeybForm::~KeybForm() {
    delete ui;
}

// assign each key-button to an enum value (buttonId) corresponding to its key
// code
void KeybForm::setupButtonGroup() {
#define X(b, e) ui->keybButtons->setId((b), (e));
    // F row
    X(ui->btn_esc, K_ESC);
    X(ui->btn_f1, K_F1);
    X(ui->btn_f2, K_F2);
    X(ui->btn_f3, K_F3);
    X(ui->btn_f4, K_F4);
    X(ui->btn_f5, K_F5);
    X(ui->btn_f6, K_F6);
    X(ui->btn_f7, K_F7);
    X(ui->btn_f8, K_F8);
    X(ui->btn_f9, K_F9);
    X(ui->btn_f10, K_F10);
    X(ui->btn_f11, K_F11);
    X(ui->btn_f12, K_F12);
    X(ui->btn_scrlk, K_SCRLK);
    X(ui->btn_pause, K_PAUSE);
    X(ui->btn_tilde, K_TILDE);
    // row 0
    X(ui->btn_1, K_1);
    X(ui->btn_2, K_2);
    X(ui->btn_3, K_3);
    X(ui->btn_4, K_4);
    X(ui->btn_5, K_5);
    X(ui->btn_6, K_6);
    X(ui->btn_7, K_7);
    X(ui->btn_8, K_8);
    X(ui->btn_9, K_9);
    X(ui->btn_0, K_0);
    X(ui->btn_minus, K_MINUS);
    X(ui->btn_eq, K_EQ);
    X(ui->btn_bkspc, K_BKSPC);
    // row 1
    X(ui->btn_tab, K_TAB);
    X(ui->btn_q, K_Q);
    X(ui->btn_w, K_W);
    X(ui->btn_e, K_E);
    X(ui->btn_r, K_R);
    X(ui->btn_t, K_T);
    X(ui->btn_y, K_Y);
    X(ui->btn_u, K_U);
    X(ui->btn_i, K_I);
    X(ui->btn_o, K_O);
    X(ui->btn_p, K_P);
    X(ui->btn_lb, K_LB);
    X(ui->btn_rb, K_RB);
    X(ui->btn_bslsh, K_BSLSH);
    // row 2
    X(ui->btn_capslk, K_CAPSLK);
    X(ui->btn_a, K_A);
    X(ui->btn_s, K_S);
    X(ui->btn_d, K_D);
    X(ui->btn_f, K_F);
    X(ui->btn_g, K_G);
    X(ui->btn_h, K_H);
    X(ui->btn_j, K_J);
    X(ui->btn_k, K_K);
    X(ui->btn_l, K_L);
    X(ui->btn_semi, K_SEMI);
    X(ui->btn_apos, K_APOS);
    X(ui->btn_enter, K_ENTER);
    // row 3
    X(ui->btn_lshift, K_LSHFT);
    X(ui->btn_z, K_Z);
    X(ui->btn_x, K_X);
    X(ui->btn_c, K_C);
    X(ui->btn_v, K_V);
    X(ui->btn_b, K_B);
    X(ui->btn_n, K_N);
    X(ui->btn_m, K_M);
    X(ui->btn_comma, K_COMMA);
    X(ui->btn_dot, K_DOT);
    X(ui->btn_fslsh, K_FSLSH);
    X(ui->btn_rshift, K_RSHFT);
    // row 4
    X(ui->btn_lctrl, K_LCTRL);
    X(ui->btn_lsup, K_LSUP);
    X(ui->btn_lalt, K_LALT);
    X(ui->btn_space, K_SPACE);
    X(ui->btn_ralt, K_RALT);
    X(ui->btn_rsup, K_RSUP);
    X(ui->btn_rctrl, K_RCTRL);
    // middle section
    X(ui->btn_ins, K_INS);
    X(ui->btn_home, K_HOME);
    X(ui->btn_pgup, K_PGUP);
    X(ui->btn_del, K_DEL);
    X(ui->btn_end, K_END);
    X(ui->btn_pgdn, K_PGDN);
    X(ui->btn_uparrow, K_UP);
    X(ui->btn_leftarrow, K_LEFT);
    X(ui->btn_downarrow, K_DOWN);
    X(ui->btn_rightarrow, K_RIGHT);
    // numpad
    X(ui->btn_numlk, K_NUMLK);
    X(ui->btn_kpslsh, K_KPSLSH);
    X(ui->btn_kpmult, K_KPSTAR);
    X(ui->btn_kpminus, K_KPMINUS);
    X(ui->btn_kp7, K_KP7);
    X(ui->btn_kp8, K_KP8);
    X(ui->btn_kp9, K_KP9);
    X(ui->btn_kp4, K_KP4);
    X(ui->btn_kp5, K_KP5);
    X(ui->btn_kp6, K_KP6);
    X(ui->btn_kp1, K_KP1);
    X(ui->btn_kp2, K_KP2);
    X(ui->btn_kp3, K_KP3);
    X(ui->btn_kp0, K_KP0);
    X(ui->btn_kpdot, K_KPDOT);
    X(ui->btn_kpplus, K_KPPLUS);
    X(ui->btn_kpenter, K_KPENTER);

#undef X
}
