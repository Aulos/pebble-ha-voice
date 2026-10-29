// Talks to Home Assistant on behalf of the watch.
//
// The watch sends a question in AppMessage chunks; this module reassembles it,
// posts it to the Home Assistant Assist conversation API, and sends the reply
// text back in chunks. The address and the long-lived token live on the phone,
// never on the watch.

var CHUNK_MAX = 180;
var REQUEST_TIMEOUT_MS = 20000;

// AppMessage commands, mirrored from src/c/comm.c.
var CMD_QUESTION = 1;
var CMD_PING = 2;
var CMD_OPEN_SETTINGS = 3;
var CMD_TEST = 4;
var CMD_STATUS = 10;
var CMD_ANSWER = 11;
var CMD_CONFIG_STATE = 12;

var STATUS_THINKING = 0;
var STATUS_ERROR = 1;

var settings = require('./settings');
var settingsPage = require('./settings-page');

function sendStatus(id, status, message) {
  var payload = { cmd: CMD_STATUS, id: id, status: status };
  if (message) {
    payload.text = message;
  }
  Pebble.sendAppMessage(payload);
}

function sendConfigState() {
  var configured = settings.isConfigured();
  Pebble.sendAppMessage({ cmd: CMD_CONFIG_STATE, configured: configured ? 1 : 0 });
  return configured;
}

function sendAnswer(id, text) {
  var chunks = [];
  for (var offset = 0; offset < text.length; offset += CHUNK_MAX) {
    chunks.push(text.substr(offset, CHUNK_MAX));
  }
  if (chunks.length === 0) {
    chunks.push('');
  }

  // One chunk at a time: sending a long reply in a burst overflows the
  // AppMessage queue, and the watch would then wait for chunks that never
  // arrive. Each chunk waits for the previous one to be acknowledged, with one
  // retry, before the next goes out.
  var index = 0;

  function sendNext() {
    if (index >= chunks.length) {
      return;
    }
    var seq = index;
    var message = {
      cmd: CMD_ANSWER,
      id: id,
      seq: seq,
      total: chunks.length,
      text: chunks[seq],
    };
    var retried = false;

    function attempt() {
      Pebble.sendAppMessage(
        message,
        function () {
          index = seq + 1;
          sendNext();
        },
        function () {
          if (!retried) {
            retried = true;
            attempt();
            return;
          }
          sendStatus(id, STATUS_ERROR, 'Could not reach the watch');
        }
      );
    }

    attempt();
  }

  sendNext();
}

// Pulls the spoken reply out of a Home Assistant conversation response.
function extractSpeech(body) {
  var response = body && body.response;
  if (!response) {
    return '';
  }
  var speech = response.speech || {};
  if (speech.plain && speech.plain.speech) {
    return speech.plain.speech;
  }
  if (speech.speech) {
    return speech.speech;
  }
  if (response.response_type === 'action_done') {
    return 'Done.';
  }
  return '';
}

// Pulls Home Assistant's own complaint out of an error body, so the watch can
// say what is wrong instead of just a number.
function describeError(body) {
  try {
    var parsed = JSON.parse(body);
    if (parsed && parsed.message) {
      return String(parsed.message).slice(0, 60);
    }
  } catch (e) {
    // Not JSON; fall through.
  }
  return String(body || '').slice(0, 60);
}

function askHomeAssistant(id, question) {
  var config = settings.read();
  if (!settings.isConfigured(config)) {
    sendStatus(id, STATUS_ERROR, 'Set the address and token in settings');
    return;
  }

  var url = config.baseUrl + '/api/conversation/process';
  var body = { text: question };
  if (config.pipeline) {
    body.agent_id = config.pipeline;
  }
  if (config.language) {
    body.language = config.language;
  }

  var xhr = new XMLHttpRequest();
  var finished = false;

  function fail(message) {
    if (finished) {
      return;
    }
    finished = true;
    sendStatus(id, STATUS_ERROR, message);
  }

  xhr.onload = function () {
    if (finished) {
      return;
    }
    if (xhr.status < 200 || xhr.status >= 300) {
      console.log('HA ' + xhr.status + ': ' + xhr.responseText);
      if (xhr.status === 401) {
        fail('Token rejected (401)');
      } else if (xhr.status === 404) {
        fail('Assist API not found (404)');
      } else if (xhr.status === 400) {
        fail('Bad request (400) ' + describeError(xhr.responseText));
      } else {
        fail('Home Assistant error ' + xhr.status);
      }
      return;
    }
    var parsed;
    try {
      parsed = JSON.parse(xhr.responseText);
    } catch (e) {
      fail('Bad response from Home Assistant');
      return;
    }
    var speech = extractSpeech(parsed);
    if (!speech) {
      fail('No reply from Assist');
      return;
    }
    finished = true;
    sendAnswer(id, speech);
  };

  xhr.onerror = function () {
    fail('Cannot reach Home Assistant');
  };

  xhr.open('POST', url, true);
  xhr.timeout = REQUEST_TIMEOUT_MS;
  xhr.ontimeout = function () {
    fail('Home Assistant timed out');
  };
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.setRequestHeader('Authorization', 'Bearer ' + config.token);
  sendStatus(id, STATUS_THINKING);
  xhr.send(JSON.stringify(body));
}

// Checks that the address and token work, without asking Assist anything. The
// settings page cannot do this itself: Home Assistant does not send the CORS
// headers a browser page would need, while this code runs on the phone and is
// not subject to them.
function testConnection(id) {
  var config = settings.read();
  if (!settings.isConfigured(config)) {
    sendStatus(id, STATUS_ERROR, 'Set the address and token in settings');
    return;
  }

  var xhr = new XMLHttpRequest();
  var finished = false;

  function fail(message) {
    if (finished) {
      return;
    }
    finished = true;
    sendStatus(id, STATUS_ERROR, message);
  }

  sendStatus(id, STATUS_THINKING);
  xhr.open('GET', config.baseUrl + '/api/', true);
  xhr.timeout = REQUEST_TIMEOUT_MS;
  xhr.ontimeout = function () {
    fail('Home Assistant timed out');
  };
  xhr.onerror = function () {
    fail('Cannot reach Home Assistant');
  };
  xhr.onload = function () {
    if (finished) {
      return;
    }
    if (xhr.status === 401 || xhr.status === 403) {
      fail('Token rejected (' + xhr.status + ')');
      return;
    }
    if (xhr.status < 200 || xhr.status >= 300) {
      fail('Home Assistant error ' + xhr.status);
      return;
    }
    finished = true;
    sendAnswer(id, 'Connected to Home Assistant.');
  };
  xhr.setRequestHeader('Authorization', 'Bearer ' + config.token);
  try {
    xhr.send();
  } catch (e) {
    fail('Phone could not send: ' + e);
  }
}

// ---------------------------------------------------------------------------
// Incoming AppMessage
// ---------------------------------------------------------------------------

var pending = {
  id: null,
  total: 0,
  next: 0,
  parts: [],
};

function handleQuestion(message) {
  if (message.seq === 0) {
    pending.id = message.id;
    pending.total = message.total || 1;
    pending.next = 0;
    pending.parts = [];
  }
  if (pending.id !== message.id || message.seq !== pending.next) {
    return;
  }
  pending.parts.push(message.text || '');
  pending.next += 1;
  if (pending.next >= pending.total) {
    var question = pending.parts.join('');
    pending.parts = [];
    askHomeAssistant(pending.id, question);
  }
}

function init() {
  Pebble.addEventListener('appmessage', function (event) {
    var message = event.payload || {};
    switch (message.cmd) {
      case CMD_QUESTION:
        handleQuestion(message);
        break;
      case CMD_PING:
        sendConfigState();
        break;
      case CMD_TEST:
        try {
          testConnection(message.id || 0);
        } catch (e) {
          console.log('Test connection failed: ' + e);
          sendStatus(message.id, STATUS_ERROR, 'Phone error: ' + e);
        }
        break;
      case CMD_OPEN_SETTINGS:
        // The watch asked for the settings page; open it on the phone.
        Pebble.openURL(settingsPage.buildUrl());
        break;
      default:
        break;
    }
  });
}

module.exports = {
  init: init,
  sendConfigState: sendConfigState,
  isConfigured: function () {
    return settings.isConfigured();
  },
};
