#include <pebble.h>

#include "assistant.h"
#include "main_window.h"

int main(void) {
  assistant_init();
  main_window_push();
  app_event_loop();
  assistant_deinit();
}
