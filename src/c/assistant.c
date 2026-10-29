#include "assistant.h"

#define QUESTION_MAX 512
#define ANSWER_MAX 1024
#define ERROR_MAX 128

static AssistantState s_state = AssistantIdle;
static AssistantStateHandler s_state_handler;
static void *s_state_context;
static AssistantAnswerHandler s_answer_handler;
static void *s_answer_context;

static DictationSession *s_dictation;
static AppTimer *s_pending_timer;

static char s_question[QUESTION_MAX];
static char s_answer[ANSWER_MAX];
static char s_error[ERROR_MAX];

static void prv_request(void);

static void prv_set_state(AssistantState state) {
  if (s_state == state) {
    return;
  }
  s_state = state;
  if (s_state_handler) {
    s_state_handler(state, s_state_context);
  }
}

static void prv_fail(const char *message) {
  snprintf(s_error, sizeof(s_error), "%s", message);
  prv_set_state(AssistantError);
}

static void prv_dictation_callback(DictationSession *session, DictationSessionStatus status,
                                   char *transcription, void *context) {
  switch (status) {
    case DictationSessionStatusSuccess:
      if (transcription && transcription[0]) {
        snprintf(s_question, sizeof(s_question), "%s", transcription);
        prv_request();
      } else {
        prv_set_state(AssistantIdle);
      }
      break;

    case DictationSessionStatusFailureTranscriptionRejected:
    case DictationSessionStatusFailureTranscriptionRejectedWithError:
      // The user backed out of the dictation UI: no error to report.
      prv_set_state(AssistantIdle);
      break;

    case DictationSessionStatusFailureNoSpeechDetected:
      prv_fail("No speech detected");
      break;

    case DictationSessionStatusFailureConnectivityError:
      prv_fail("Phone not connected");
      break;

    case DictationSessionStatusFailureDisabled:
      prv_fail("Dictation is off in the phone app");
      break;

    default:
      prv_fail("Could not transcribe");
      break;
  }
}

static void prv_answer_ready(void *context) {
  s_pending_timer = NULL;
  prv_set_state(AssistantIdle);
  if (s_answer_handler) {
    s_answer_handler(s_answer_context);
  }
}

static void prv_request(void) {
  prv_set_state(AssistantThinking);
  // Placeholder backend until the phone bridge lands: echo the request so the
  // screens can be exercised without a phone.
  snprintf(s_answer, sizeof(s_answer), "You said: %s", s_question);
  s_pending_timer = app_timer_register(600, prv_answer_ready, NULL);
}

void assistant_init(void) {
  s_question[0] = '\0';
  s_answer[0] = '\0';
  s_error[0] = '\0';
}

void assistant_deinit(void) {
  if (s_pending_timer) {
    app_timer_cancel(s_pending_timer);
    s_pending_timer = NULL;
  }
  if (s_dictation) {
    dictation_session_destroy(s_dictation);
    s_dictation = NULL;
  }
}

void assistant_start(void) {
  if (s_state == AssistantListening || s_state == AssistantThinking) {
    return;
  }

  if (!s_dictation) {
    s_dictation = dictation_session_create(QUESTION_MAX, prv_dictation_callback, NULL);
    if (!s_dictation) {
      prv_fail("Dictation unavailable");
      return;
    }
    dictation_session_enable_confirmation(s_dictation, false);
    dictation_session_enable_error_dialogs(s_dictation, false);
  }

  s_error[0] = '\0';
  prv_set_state(AssistantListening);
  vibes_short_pulse();

  if (dictation_session_start(s_dictation) != DictationSessionStatusSuccess) {
    prv_fail("Could not start dictation");
  }
}

void assistant_cancel(void) {
  if (s_pending_timer) {
    app_timer_cancel(s_pending_timer);
    s_pending_timer = NULL;
  }
  if (s_state == AssistantListening && s_dictation) {
    dictation_session_stop(s_dictation);
  }
  prv_set_state(AssistantIdle);
}

AssistantState assistant_get_state(void) {
  return s_state;
}

void assistant_set_state_handler(AssistantStateHandler handler, void *context) {
  s_state_handler = handler;
  s_state_context = context;
}

void assistant_set_answer_handler(AssistantAnswerHandler handler, void *context) {
  s_answer_handler = handler;
  s_answer_context = context;
}

const char *assistant_question(void) {
  return s_question;
}

const char *assistant_answer(void) {
  return s_answer;
}

const char *assistant_error(void) {
  return s_error;
}

bool assistant_has_answer(void) {
  return s_answer[0] != '\0';
}
