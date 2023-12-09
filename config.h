/*
Copyright 2012 Jun Wako <wakojun@gmail.com>
Copyright 2015 Jack Humbert

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once


// wiring of each half
#define MATRIX_ROW_PINS { D4, C6, D7, E6, B4, B5 }
#define MATRIX_COL_PINS { F5, F6, F7, B1, B3, B2, B6 }

#define DIODE_DIRECTION COL2ROW

#ifdef RGBLIGHT_ENABLE
// WS2812 RGB LED strip input and number of LEDs
#define WS2812_DI_PIN F4
#define RGBLED_NUM 24
#define RGBLIGHT_SPLIT
#define RGBLED_SPLIT { 12, 12 }
#define RGBLIGHT_LAYERS
#define RGBLIGHT_EFFECT_BREATHING
#define RGBLIGHT_EFFECT_RAINBOW_MOOD
#define RGBLIGHT_EFFECT_RAINBOW_SWIRL
#define RGBLIGHT_DEFAULT_MODE RGBLIGHT_MODE_RAINBOW_MOOD
#define RGBLIGHT_SLEEP
#define RGBLIGHT_DEFAULT_HUE 127
// #define RGBLIGHT_HUE_STEP 8
// #define RGBLIGHT_SAT_STEP 8
// #define RGBLIGHT_VAL_STEP 8
#endif

/* PS/2 mouse interrupt version */
#ifdef PS2_DRIVER_INTERRUPT
/* uses INT1 for clock line(ATMega32U4) */
#    define PS2_CLOCK_PIN  D3
#    define PS2_DATA_PIN   D2

#    define PS2_INT_INIT()                          \
        do {                                        \
            EICRA |= ((1 << ISC31) | (0 << ISC30)); \
        } while (0)
#    define PS2_INT_ON()          \
        do {                      \
            EIMSK |= (1 << INT3); \
        } while (0)
#    define PS2_INT_OFF()          \
        do {                       \
            EIMSK &= ~(1 << INT3); \
        } while (0)
#    define PS2_INT_VECT INT3_vect
#    define PS2_MOUSE_USE_REMOTE_MODE
#endif
