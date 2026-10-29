#pragma once
#include <pebble.h>

// Shows the answer to the last question. The screen supports scrolling with the
// up/down buttons, a touch drag, and asking again with SELECT or a tap on the
// footer button.
void answer_window_push(void);
