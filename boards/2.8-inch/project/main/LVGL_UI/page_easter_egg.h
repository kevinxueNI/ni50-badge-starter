#pragma once

#include "lvgl.h"

/**
 * page_easter_egg.h — Hidden game space (Easter Egg)
 *
 * Entry: Long press (2s) on "Kevin Xue" name label on Card page.
 * Layout: Independent full screen (not in tileview).
 * Contains a game selection menu: NI Trivia or NI Lab (System Builder).
 * Exit: LabVIEW red STOP button returns to main screen.
 */

/**
 * Show the easter egg game selection menu.
 * Creates a new screen and loads it.
 */
void easter_egg_show(void);

/**
 * Return from easter egg to main screen.
 * Can be called from any game sub-page.
 */
void easter_egg_exit(void);
