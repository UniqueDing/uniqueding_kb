#include QMK_KEYBOARD_H

// Each layer gets a name for readability, which is then used in the keymap matrix below.
// The underscores don't mean anything - you can have a layer called STUFF or any other name.
// Layer names don't all need to be of the same length, obviously, and you can also skip them
// entirely and just use numbers.

// Some basic macros
#define TASK   LCTL(LSFT(KC_ESC))
#define TAB_R  LCTL(KC_TAB)
#define TAB_L  LCTL(LSFT(KC_TAB))
#define TAB_RO LCTL(LSFT(KC_T))
#define A_S A(KC_LSFT)
#define C_S C(KC_LSFT)
#define LAYER0 TO(0)

#define VMUTE KC_KB_MUTE
#define VUP KC_KB_VOLUME_UP
#define VDOWN KC_KB_VOLUME_DOWN

enum layers {
    _QWERTY,
    _FN,
    _NUMPAD
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_QWERTY] = LAYOUT(
        // left hand
        KC_GRV,        KC_1,    KC_2,    KC_3,   KC_4,   KC_5,   KC_ESC,
        KC_TAB,        KC_Q,    KC_W,    KC_E,   KC_R,   KC_T,   TG(_NUMPAD),
        KC_CAPS,       KC_A,    KC_S,    KC_D,   KC_F,   KC_G,   TG(_FN),
        KC_LSFT,      KC_Z,    KC_X,    KC_C,   KC_V,   KC_B,
        TT(_NUMPAD),   KC_LCTL, KC_LBRC, KC_RBRC,
                                    KC_SPC,  KC_LGUI,
                                    KC_LALT, C_S,
                                    KC_LEFT, KC_RGHT,
        // right hand
                          KC_DEL,      KC_6,    KC_7,    KC_8,     KC_9,     KC_0,     KC_BSPC,
                          TG(_NUMPAD), KC_Y,    KC_U,    KC_I,     KC_O,     KC_P,     KC_BSLS,
                          TG(_FN),     KC_H,    KC_J,    KC_K,     KC_L,     KC_SCLN,  KC_QUOT,
                                       KC_N,    KC_M,    KC_COMM,  KC_DOT,   KC_SLSH,  KC_RSFT,
                                                KC_MINS, KC_EQL,   KC_RCTL,  TT(_FN),
             _______, KC_ENT,
             A_S,     KC_BSPC,
             KC_UP,   KC_DOWN
    ),

    [_FN] = LAYOUT(
        // left hand
        _______,   _______,   _______,   _______,   _______,   _______,  _______,
        _______,   _______,   _______,   _______,   _______,   _______,  LAYER0,
        _______,   _______,   _______,   _______,   _______,   _______,  LAYER0,
        _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,
                                    _______, _______,
                                    _______, _______,
                                    _______, _______,
        // right hand
                          _______,   _______,   _______,   _______,   _______,   _______,   QK_RBT,
                          LAYER0,    _______,   _______,   _______,   _______,   _______,   EE_CLR,
                          LAYER0,    _______,   KC_BTN1,   KC_BTN3,   KC_BTN2,   _______,   QK_BOOT,
                                     _______,   _______,   _______,   _______,   _______,   _______,
                                                           _______,   _______,   _______,   _______,
             _______, _______,
             _______, _______,
             _______, _______
    ),

    [_NUMPAD] = LAYOUT(
        // left hand
        KC_ESC,    KC_F1,     KC_F2,      KC_F3,    KC_F4,     KC_F5,    KC_F6,
        _______,   VMUTE,     VUP,       VDOWN,     _______,   _______,  LAYER0,
        _______,   RGB_TOG,   RGB_MOD,   RGB_VAI,   RGB_VAD,   _______,  LAYER0,
        _______,   RGB_HUI,   RGB_HUD,   RGB_SAI,   RGB_SAD,   _______,
        KC_MSTP,   KC_MPLY,   KC_MPRV,   KC_MNXT,
                                    _______, _______,
                                    _______, _______,
                                    _______, _______,
        // right hand
                          KC_F7,     KC_F8,     KC_F9,     KC_F10,    KC_F11,    KC_F12,    _______,
                          LAYER0,    _______,   KC_P7,     KC_P8,     KC_P9,     KC_NUM,   _______,
                          LAYER0,    _______,   KC_P4,     KC_P5,     KC_P6,     KC_PAST,   _______,
                                     _______,   KC_P1,     KC_P2,     KC_P3,     KC_PSLS,   _______,
                                                           KC_P0,     KC_PDOT,   _______,   _______,
             _______, KC_PENT,
             KC_BSPC, _______,
             _______, _______
    )
};

#ifdef RGBLIGHT_ENABLE
const rgblight_segment_t PROGMEM layer_qwert[] = RGBLIGHT_LAYER_SEGMENTS(
    {0, 24, HSV_CYAN}
);
const rgblight_segment_t PROGMEM layer_fn[] = RGBLIGHT_LAYER_SEGMENTS(
    {8, 4, HSV_PINK},
    {20, 4, HSV_PINK}
);
const rgblight_segment_t PROGMEM layer_numpad[] = RGBLIGHT_LAYER_SEGMENTS(
    {8, 4, HSV_ORANGE},
    {20, 4, HSV_ORANGE}
);

// Now define the array of layers. Later layers take precedence
const rgblight_segment_t* const PROGMEM my_rgb_layers[] = RGBLIGHT_LAYERS_LIST(
    layer_qwert,
    layer_fn,
    layer_numpad
);

void keyboard_post_init_user(void) {
    // Enable the LED layers
    rgblight_layers = my_rgb_layers;

}


// layer_state_t default_layer_state_set_user(layer_state_t state) {
//     rgblight_set_layer_state(0, layer_state_cmp(state, _QWERTY));
//     return state;
// }

layer_state_t layer_state_set_user(layer_state_t state) {
    // rgblight_set_layer_state(0, layer_state_cmp(state, _QWERTY));
    rgblight_set_layer_state(1, layer_state_cmp(state, _FN));
    rgblight_set_layer_state(2, layer_state_cmp(state, _NUMPAD));
    return state;
}
#endif
