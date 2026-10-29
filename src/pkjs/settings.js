// Settings shared by the Home Assistant bridge and the settings page.

var STORAGE_KEY = 'clay-settings';

function normalizeBaseUrl(baseUrl) {
  var url = (baseUrl || '').trim();
  if (!url) {
    return '';
  }
  if (!/^https?:\/\//i.test(url)) {
    url = 'http://' + url;
  }
  return url.replace(/\/+$/, '');
}

// Home Assistant wants a language code such as "en", not a language name. A
// name makes Assist answer "I could not understand that" instead of failing
// loudly, so the obvious wrong values are turned into "use the default".
function normalizeLanguage(language) {
  var value = (language || '').trim();
  if (!value) {
    return '';
  }
  if (/^[a-z]{2,3}(-[a-z]{2,4})?$/i.test(value) === false) {
    return '';
  }
  return value.toLowerCase();
}

// The pipeline field holds an agent id. "preferred" and "default" look like
// sensible values but Home Assistant rejects them with a 400, so they mean
// "leave it out".
function normalizeAgent(pipeline) {
  var value = (pipeline || '').trim();
  var lower = value.toLowerCase();
  if (lower === 'preferred' || lower === 'default' || lower === 'none') {
    return '';
  }
  return value;
}

function read() {
  var raw = {};
  try {
    raw = JSON.parse(localStorage.getItem(STORAGE_KEY) || '{}') || {};
  } catch (e) {
    console.log('Could not read settings: ' + e);
    raw = {};
  }
  return {
    baseUrl: normalizeBaseUrl(raw.baseUrl),
    token: (raw.token || '').trim(),
    language: normalizeLanguage(raw.language),
    pipeline: normalizeAgent(raw.pipeline),
  };
}

function write(values) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(values));
}

function isConfigured(settings) {
  var s = settings || read();
  return !!(s.baseUrl && s.token);
}

module.exports = {
  read: read,
  write: write,
  normalizeLanguage: normalizeLanguage,
  normalizeAgent: normalizeAgent,
  isConfigured: isConfigured,
  normalizeBaseUrl: normalizeBaseUrl,
};
