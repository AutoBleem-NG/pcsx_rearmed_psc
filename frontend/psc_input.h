/*
 * PlayStation Classic (PSC) Controller Input Mappings
 * (C) 2025 AutoBleem-NG Team
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 *
 * PSC controllers are USB HID joysticks handled by SDL2's joystick subsystem.
 * SDL reports joystick button presses which libpicofe's in_sdl.c translates
 * to keycodes in the 0xF0-0xF9 range (see SDLK_JOY_BASE in in_sdl.c).
 *
 * Physical PSC controller button layout:
 *   Button 0 = Triangle    -> keycode 0xF0
 *   Button 1 = Circle      -> keycode 0xF1
 *   Button 2 = Cross       -> keycode 0xF2
 *   Button 3 = Square      -> keycode 0xF3
 *   Button 4 = L2          -> keycode 0xF4
 *   Button 5 = R2          -> keycode 0xF5
 *   Button 6 = L1          -> keycode 0xF6
 *   Button 7 = R1          -> keycode 0xF7
 *   Button 8 = Select      -> keycode 0xF8
 *   Button 9 = Start       -> keycode 0xF9
 *
 * D-pad is reported as joystick axes (not hat), handled by in_sdl.c axis code.
 */

#ifndef PSC_INPUT_H
#define PSC_INPUT_H

/* PSC controller button keycodes (SDL joystick button -> keycode mapping) */
#define PSC_KEY_TRIANGLE  0xF0
#define PSC_KEY_CIRCLE    0xF1
#define PSC_KEY_CROSS     0xF2
#define PSC_KEY_SQUARE    0xF3
#define PSC_KEY_L2        0xF4
#define PSC_KEY_R2        0xF5
#define PSC_KEY_L1        0xF6
#define PSC_KEY_R1        0xF7
#define PSC_KEY_SELECT    0xF8
#define PSC_KEY_START     0xF9

/*
 * Menu button mappings for PSC controller:
 *   Cross (0xF2)    = OK/Confirm
 *   Circle (0xF1)   = Back/Cancel
 *   Square (0xF3)   = Menu Action 2
 *   Triangle (0xF0) = Menu Action 3
 *   Select (0xF8)   = Enter emulator menu
 */
#define PSC_MENU_OK       PSC_KEY_CROSS
#define PSC_MENU_BACK     PSC_KEY_CIRCLE
#define PSC_MENU_ACTION2  PSC_KEY_SQUARE
#define PSC_MENU_ACTION3  PSC_KEY_TRIANGLE
#define PSC_MENU_ENTER    PSC_KEY_SELECT

/*
 * Select+Start combo for entering menu.
 * PSC controllers have no dedicated menu button, so we use this combo.
 * DKEY_SELECT=0, DKEY_START=3 (from plugin_lib.h)
 */
#define PSC_MENU_COMBO_MASK  ((1 << 0) | (1 << 3))  /* SELECT | START */
#define PSC_IS_MENU_COMBO(keys) (((keys) & PSC_MENU_COMBO_MASK) == PSC_MENU_COMBO_MASK)

#endif /* PSC_INPUT_H */
