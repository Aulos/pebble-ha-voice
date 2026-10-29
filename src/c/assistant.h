#pragma once
#include <pebble.h>

// Drives one voice request end to end: dictation -> Home Assistant -> answer.
// The screens only observe this module through the handlers below.

typedef enum {
  AssistantIdle,
  AssistantListening,
  AssistantThinking,
  AssistantError,
} AssistantState;

typedef void (*AssistantStateHandler)(AssistantState state, void *context);
typedef void (*AssistantAnswerHandler)(void *context);

void assistant_init(void);
void assistant_deinit(void);

// Starts a dictation session. Ignored while a request is already running.
void assistant_start(void);

// Abandons the request in progress and returns to idle.
void assistant_cancel(void);

AssistantState assistant_get_state(void);

void assistant_set_state_handler(AssistantStateHandler handler, void *context);
void assistant_set_answer_handler(AssistantAnswerHandler handler, void *context);

const char *assistant_question(void);
const char *assistant_answer(void);
const char *assistant_error(void);
bool assistant_has_answer(void);
