#include "keybform.h"
#include "ui_keybform.h"

KeybForm::KeybForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::KeybForm)
{
    ui->setupUi(this);
    setupButtonGroup();
}

// assign each key-button to an enum value (buttonId) corresponding to its key code
void KeybForm::setupButtonGroup()
{
    //F row
    ui->keybButtons->setId(ui->btn_esc, K_ESC);
    ui->keybButtons->setId(ui->btn_f1, K_F1);
    ui->keybButtons->setId(ui->btn_f2, K_F2);
    ui->keybButtons->setId(ui->btn_f3, K_F3);
    ui->keybButtons->setId(ui->btn_f4, K_F4);
    ui->keybButtons->setId(ui->btn_f5, K_F5);
    ui->keybButtons->setId(ui->btn_f6, K_F6);
    ui->keybButtons->setId(ui->btn_f7, K_F7);
    ui->keybButtons->setId(ui->btn_f8, K_F8);
    ui->keybButtons->setId(ui->btn_f9, K_F9);
    ui->keybButtons->setId(ui->btn_f10, K_F10);
    ui->keybButtons->setId(ui->btn_f11, K_F11);
    ui->keybButtons->setId(ui->btn_f12, K_F12);
    ui->keybButtons->setId(ui->btn_scrlk, K_SCRLK);
    ui->keybButtons->setId(ui->btn_pause, K_PAUSE);
    ui->keybButtons->setId(ui->btn_tilde, K_TILDE);
    //row 0
    ui->keybButtons->setId(ui->btn_1, K_1);
    ui->keybButtons->setId(ui->btn_2, K_2);
    ui->keybButtons->setId(ui->btn_3, K_3);
    ui->keybButtons->setId(ui->btn_4, K_4);
    ui->keybButtons->setId(ui->btn_5, K_5);
    ui->keybButtons->setId(ui->btn_6, K_6);
    ui->keybButtons->setId(ui->btn_7, K_7);
    ui->keybButtons->setId(ui->btn_8, K_8);
    ui->keybButtons->setId(ui->btn_9, K_9);
    ui->keybButtons->setId(ui->btn_0, K_0);
    ui->keybButtons->setId(ui->btn_minus, K_MINUS);
    ui->keybButtons->setId(ui->btn_eq, K_EQ);
    ui->keybButtons->setId(ui->btn_bkspc, K_BKSPC);
    //row 1
    ui->keybButtons->setId(ui->btn_tab, K_TAB);
    ui->keybButtons->setId(ui->btn_q, K_Q);
    ui->keybButtons->setId(ui->btn_w, K_W);
    ui->keybButtons->setId(ui->btn_e, K_E);
    ui->keybButtons->setId(ui->btn_r, K_R);
    ui->keybButtons->setId(ui->btn_t, K_T);
    ui->keybButtons->setId(ui->btn_y, K_Y);
    ui->keybButtons->setId(ui->btn_u, K_U);
    ui->keybButtons->setId(ui->btn_i, K_I);
    ui->keybButtons->setId(ui->btn_o, K_O);
    ui->keybButtons->setId(ui->btn_p, K_P);
    ui->keybButtons->setId(ui->btn_lb, K_LB);
    ui->keybButtons->setId(ui->btn_rb, K_RB);
    ui->keybButtons->setId(ui->btn_bslsh, K_BSLSH);
    //row 2
    ui->keybButtons->setId(ui->btn_capslk, K_CAPSLK);
    ui->keybButtons->setId(ui->btn_a, K_A);
    ui->keybButtons->setId(ui->btn_s, K_S);
    ui->keybButtons->setId(ui->btn_d, K_D);
    ui->keybButtons->setId(ui->btn_f, K_F);
    ui->keybButtons->setId(ui->btn_g, K_G);
    ui->keybButtons->setId(ui->btn_h, K_H);
    ui->keybButtons->setId(ui->btn_j, K_J);
    ui->keybButtons->setId(ui->btn_k, K_K);
    ui->keybButtons->setId(ui->btn_l, K_L);
    ui->keybButtons->setId(ui->btn_semi, K_SEMI);
    ui->keybButtons->setId(ui->btn_apos, K_APOS);
    ui->keybButtons->setId(ui->btn_enter, K_ENTER);
    //row 3
    ui->keybButtons->setId(ui->btn_lshift, K_LSHFT);
    ui->keybButtons->setId(ui->btn_z, K_Z);
    ui->keybButtons->setId(ui->btn_x, K_X);
    ui->keybButtons->setId(ui->btn_c, K_C);
    ui->keybButtons->setId(ui->btn_v, K_V);
    ui->keybButtons->setId(ui->btn_b, K_B);
    ui->keybButtons->setId(ui->btn_n, K_N);
    ui->keybButtons->setId(ui->btn_m, K_M);
    ui->keybButtons->setId(ui->btn_comma, K_COMMA);
    ui->keybButtons->setId(ui->btn_dot, K_DOT);
    ui->keybButtons->setId(ui->btn_fslsh, K_FSLSH);
    ui->keybButtons->setId(ui->btn_rshift, K_RSHFT);
    //row 4
    ui->keybButtons->setId(ui->btn_lctrl, K_LCTRL);
    ui->keybButtons->setId(ui->btn_lsup, K_LSUP);
    ui->keybButtons->setId(ui->btn_lalt, K_LALT);
    ui->keybButtons->setId(ui->btn_space, K_SPACE);
    ui->keybButtons->setId(ui->btn_ralt, K_RALT);
    ui->keybButtons->setId(ui->btn_rsup, K_RSUP);
    ui->keybButtons->setId(ui->btn_rctrl, K_RCTRL);
    //middle section
    ui->keybButtons->setId(ui->btn_ins, K_INS);
    ui->keybButtons->setId(ui->btn_home, K_HOME);
    ui->keybButtons->setId(ui->btn_pgup, K_PGUP);
    ui->keybButtons->setId(ui->btn_del, K_DEL);
    ui->keybButtons->setId(ui->btn_end, K_END);
    ui->keybButtons->setId(ui->btn_pgdn, K_PGDN);
    ui->keybButtons->setId(ui->btn_uparrow, K_UP);
    ui->keybButtons->setId(ui->btn_leftarrow, K_LEFT);
    ui->keybButtons->setId(ui->btn_downarrow, K_DOWN);
    ui->keybButtons->setId(ui->btn_rightarrow, K_RIGHT);
    //numpad
    ui->keybButtons->setId(ui->btn_numlk, K_NUMLK);
    ui->keybButtons->setId(ui->btn_kpslsh, K_KPSLSH);
    ui->keybButtons->setId(ui->btn_kpmult, K_KPSTAR);
    ui->keybButtons->setId(ui->btn_kpminus, K_KPMINUS);
    ui->keybButtons->setId(ui->btn_kp7, K_KP7);
    ui->keybButtons->setId(ui->btn_kp8, K_KP8);
    ui->keybButtons->setId(ui->btn_kp9, K_KP9);
    ui->keybButtons->setId(ui->btn_kp4, K_KP4);
    ui->keybButtons->setId(ui->btn_kp5, K_KP5);
    ui->keybButtons->setId(ui->btn_kp6, K_KP6);
    ui->keybButtons->setId(ui->btn_kp1, K_KP1);
    ui->keybButtons->setId(ui->btn_kp2, K_KP2);
    ui->keybButtons->setId(ui->btn_kp3, K_KP3);
    ui->keybButtons->setId(ui->btn_kp0, K_KP0);
    ui->keybButtons->setId(ui->btn_kpdot, K_KPDOT);
    ui->keybButtons->setId(ui->btn_kpplus, K_KPPLUS);
    ui->keybButtons->setId(ui->btn_kpenter, K_KPENTER);
}

void KeybForm::on_keybButtons_buttonClicked(int id)
{
    KeyId kid = static_cast<KeyId>(id);
    printf("%s = %d\n", KEY_CODES[kid].toStdString().c_str(), id);
    fflush(stdout);
}

KeybForm::~KeybForm()
{
    delete ui;
}
