#pragma once
#include <pebble.h>

void main_window_push(void);

// Starts a new voice request from the main screen. Used by other screens that
// hand control back for another question.
void main_window_ask(void);
