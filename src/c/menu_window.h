#pragma once
#include <pebble.h>

// The watch-side menu: ask again, or open the settings page on the phone.
// Every other screen is a single action, so this is where the app keeps the
// entry point that the phone cannot offer on its own.

void menu_window_push(void);
