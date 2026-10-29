#include "comm.h"

// Chunk size in bytes. AppMessage strings are short by design, and this leaves
// room for the other keys in the dictionary.
#define CHUNK_MAX 180
#define QUESTION_MAX 512
#define ANSWER_MAX 1024
#define ERROR_MAX 128

#define CMD_QUESTION 1
#define CMD_PING 2
#define CMD_OPEN_SETTINGS 3

#define CMD_STATUS 10
#define CMD_ANSWER 11
#define CMD_CONFIG_STATE 12

#define STATUS_THINKING 0
#define STATUS_ERROR 1

static CommHandlers s_handlers;
static bool s_ready;

// Outgoing question, sent chunk by chunk from the outbox-sent callback.
static char s_out_text[QUESTION_MAX];
static uint32_t s_out_id;
static uint16_t s_out_total;
static uint16_t s_out_next;
static bool s_out_active;

// Incoming answer assembly.
static char s_in_text[ANSWER_MAX];
static bool s_in_truncated;
static uint16_t s_in_len;
static uint16_t s_in_total;
static uint16_t s_in_next;
static uint32_t s_in_id;
static bool s_in_active;

static bool s_configured;

// ---------------------------------------------------------------------------
// Sending
// ---------------------------------------------------------------------------

static void prv_send_next_chunk(void) {
  if (!s_out_active || !s_ready) {
    return;
  }

  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    // Try again when the outbox frees up.
    return;
  }

  size_t offset = (size_t)s_out_next * CHUNK_MAX;
  size_t remaining = strlen(s_out_text + offset);
  size_t length = remaining > CHUNK_MAX ? CHUNK_MAX : remaining;

  char chunk[CHUNK_MAX + 1];
  memcpy(chunk, s_out_text + offset, length);
  chunk[length] = '\0';

  dict_write_int32(iter, MESSAGE_KEY_cmd, CMD_QUESTION);
  dict_write_int32(iter, MESSAGE_KEY_id, (int32_t)s_out_id);
  dict_write_int32(iter, MESSAGE_KEY_seq, s_out_next);
  dict_write_int32(iter, MESSAGE_KEY_total, s_out_total);
  dict_write_cstring(iter, MESSAGE_KEY_text, chunk);

  if (app_message_outbox_send() != APP_MSG_OK) {
    return;
  }
  s_out_next++;
  if (s_out_next >= s_out_total) {
    s_out_active = false;
  }
}

static void prv_outbox_sent(DictionaryIterator *iter, void *context) {
  prv_send_next_chunk();
}

static void prv_outbox_failed(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  // A lost question is worth showing. A lost ping or settings request is not:
  // those happen whenever the phone is away, and the main screen already says
  // that the app is not set up.
  bool was_question = s_out_active;
  s_out_active = false;
  if (was_question && s_handlers.on_error) {
    s_handlers.on_error("Phone not connected", s_handlers.context);
  }
}

static void prv_send_simple(int cmd) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, MESSAGE_KEY_cmd, cmd);
  app_message_outbox_send();
}

// ---------------------------------------------------------------------------
// Receiving
// ---------------------------------------------------------------------------

static void prv_handle_status(DictionaryIterator *iter, int32_t status) {
  if (status == STATUS_THINKING) {
    if (s_handlers.on_thinking) {
      s_handlers.on_thinking(s_handlers.context);
    }
    return;
  }

  Tuple *text = dict_find(iter, MESSAGE_KEY_text);
  const char *message = text ? text->value->cstring : "Request failed";
  s_out_active = false;
  s_in_active = false;
  if (s_handlers.on_error) {
    s_handlers.on_error(message, s_handlers.context);
  }
}

static void prv_handle_answer_chunk(DictionaryIterator *iter, int32_t seq, int32_t total,
                                    int32_t id, const char *chunk) {
  if (!chunk) {
    return;
  }

  if (seq == 0) {
    s_in_len = 0;
    s_in_total = total > 0 ? total : 1;
    s_in_next = 0;
    s_in_id = (uint32_t)id;
    s_in_active = true;
    s_in_truncated = false;
    s_in_text[0] = '\0';
  }

  if (!s_in_active || (uint16_t)seq != s_in_next) {
    return;
  }

  size_t length = strlen(chunk);
  if (s_in_len + length >= ANSWER_MAX) {
    // The reply is longer than the watch keeps. Cut it, and say so with an
    // ellipsis rather than ending mid-sentence with no hint. Four bytes are
    // kept back for that ellipsis and the terminator.
    length = (ANSWER_MAX - 4 > s_in_len) ? ANSWER_MAX - 4 - s_in_len : 0;
    s_in_truncated = true;
  }
  memcpy(s_in_text + s_in_len, chunk, length);
  s_in_len += length;
  s_in_text[s_in_len] = '\0';
  s_in_next++;

  if (s_in_next >= s_in_total) {
    s_in_active = false;
    if (s_in_truncated) {
      memcpy(s_in_text + s_in_len, "...", 4);
      s_in_len += 3;
      s_in_truncated = false;
    }
    if (s_handlers.on_answer) {
      s_handlers.on_answer(s_in_text, s_handlers.context);
    }
  }
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *cmd_tuple = dict_find(iter, MESSAGE_KEY_cmd);
  if (!cmd_tuple) {
    return;
  }
  int32_t cmd = cmd_tuple->value->int32;

  Tuple *id_tuple = dict_find(iter, MESSAGE_KEY_id);
  int32_t id = id_tuple ? id_tuple->value->int32 : 0;

  switch (cmd) {
    case CMD_STATUS: {
      Tuple *status = dict_find(iter, MESSAGE_KEY_status);
      prv_handle_status(iter, status ? status->value->int32 : STATUS_ERROR);
      break;
    }
    case CMD_ANSWER: {
      Tuple *seq = dict_find(iter, MESSAGE_KEY_seq);
      Tuple *total = dict_find(iter, MESSAGE_KEY_total);
      Tuple *text = dict_find(iter, MESSAGE_KEY_text);
      prv_handle_answer_chunk(iter, seq ? seq->value->int32 : 0,
                              total ? total->value->int32 : 1, id,
                              text ? text->value->cstring : NULL);
      break;
    }
    case CMD_CONFIG_STATE: {
      Tuple *configured = dict_find(iter, MESSAGE_KEY_configured);
      s_configured = configured ? configured->value->int32 != 0 : false;
      if (s_handlers.on_config_state) {
        s_handlers.on_config_state(s_configured, s_handlers.context);
      }
      break;
    }
    default:
      break;
  }
}

static void prv_inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "inbox dropped: %d", (int)reason);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void comm_init(const CommHandlers *handlers) {
  s_handlers = *handlers;

  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_sent(prv_outbox_sent);
  app_message_register_outbox_failed(prv_outbox_failed);

  if (app_message_open(1024, 1024) == APP_MSG_OK) {
    s_ready = true;
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "app_message_open failed");
  }
}

void comm_deinit(void) {
  app_message_deregister_callbacks();
  s_ready = false;
}

void comm_send_question(const char *text) {
  if (!text || !text[0]) {
    return;
  }

  snprintf(s_out_text, sizeof(s_out_text), "%s", text);
  size_t length = strlen(s_out_text);
  s_out_total = (length + CHUNK_MAX - 1) / CHUNK_MAX;
  if (s_out_total == 0) {
    s_out_total = 1;
  }
  s_out_next = 0;
  s_out_id++;
  s_out_active = true;

  prv_send_next_chunk();
}

void comm_request_config_state(void) {
  prv_send_simple(CMD_PING);
}

void comm_open_settings(void) {
  prv_send_simple(CMD_OPEN_SETTINGS);
}
