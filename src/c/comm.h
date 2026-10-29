#pragma once
#include <pebble.h>

// AppMessage transport between the watch and the phone-side JavaScript.
//
// A question travels to the phone in chunks, and the answer comes back the same
// way, so long transcriptions and long answers are not limited by the size of a
// single AppMessage. The phone is also the only side that knows the Home
// Assistant address and token.

typedef struct {
  void (*on_thinking)(void *context);
  void (*on_answer)(const char *text, void *context);
  void (*on_error)(const char *message, void *context);
  void (*on_config_state)(bool configured, void *context);
  void *context;
} CommHandlers;

void comm_init(const CommHandlers *handlers);
void comm_deinit(void);

// Queues a question for the phone. Chunks are sent one at a time.
void comm_send_question(const char *text);

// Asks the phone whether an address and token have been set.
void comm_request_config_state(void);

// Opens the settings page on the phone.
void comm_open_settings(void);
