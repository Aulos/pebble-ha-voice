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
    language: (raw.language || '').trim(),
    pipeline: (raw.pipeline || '').trim(),
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
  isConfigured: isConfigured,
  normalizeBaseUrl: normalizeBaseUrl,
};
