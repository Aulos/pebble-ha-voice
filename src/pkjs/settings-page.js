// The settings page shown on the phone.
//
// The page is built as a data URL, so it needs no hosting: Pebble.openURL opens
// it in the phone's webview, and it hands the values back by navigating to
// pebblejs://close#<encoded JSON>, which fires the webviewclosed event.

var settings = require('./settings');

var FIELDS = [
  {
    key: 'baseUrl',
    label: 'Home Assistant address',
    placeholder: 'http://homeassistant.local:8123',
    type: 'url',
    hint: 'Include the port, for example :8123. https:// works too.',
  },
  {
    key: 'token',
    label: 'Long-lived access token',
    placeholder: 'Paste the token here',
    type: 'password',
    hint: 'Create one in Home Assistant under your profile, Security, Long-lived access tokens.',
  },
  {
    key: 'language',
    label: 'Language (optional)',
    placeholder: 'en',
    type: 'text',
    hint: 'A code such as en, de or pl, not a language name. Leave empty for the default.',
  },
  {
    key: 'pipeline',
    label: 'Assist pipeline (optional)',
    placeholder: 'conversation.home_assistant',
    type: 'text',
    hint: 'The agent id from Assist settings, such as conversation.home_assistant. Leave empty to use the preferred agent.',
  },
];

function escapeHtml(value) {
  return String(value)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function buildHtml(values) {
  var fields = FIELDS.map(function (field) {
    return (
      '<label class="field">' +
      '<span class="label">' + escapeHtml(field.label) + '</span>' +
      '<input type="' + field.type + '" name="' + field.key + '"' +
      ' value="' + escapeHtml(values[field.key] || '') + '"' +
      ' placeholder="' + escapeHtml(field.placeholder) + '"' +
      ' autocapitalize="off" autocorrect="off" spellcheck="false">' +
      '<span class="hint">' + escapeHtml(field.hint) + '</span>' +
      '</label>'
    );
  }).join('');

  return (
    '<!DOCTYPE html><html><head><meta charset="utf-8">' +
    '<meta name="viewport" content="width=device-width,initial-scale=1">' +
    '<title>HA Voice settings</title><style>' +
    'body{margin:0;padding:20px 16px 96px;background:#0e1114;color:#e8edf2;' +
    'font:16px/1.5 -apple-system,"Segoe UI",Roboto,sans-serif}' +
    'h1{font-size:20px;margin:0 0 4px}' +
    'p.sub{color:#93a1ad;font-size:14px;margin:0 0 22px}' +
    '.field{display:block;margin-bottom:20px}' +
    '.label{display:block;font-size:14px;font-weight:600;margin-bottom:6px}' +
    'input{width:100%;box-sizing:border-box;padding:12px;border-radius:10px;' +
    'border:1px solid #2a333d;background:#151b21;color:#e8edf2;font-size:16px}' +
    'input:focus{outline:none;border-color:#00aaff}' +
    '.hint{display:block;color:#7d8894;font-size:12.5px;margin-top:6px}' +
    'button{position:fixed;left:0;right:0;bottom:0;padding:16px;border:0;' +
    'background:#00aaff;color:#04121c;font-size:17px;font-weight:700}' +
    '</style></head><body>' +
    '<h1>HA Voice</h1>' +
    '<p class="sub">Ask Home Assistant by talking to your watch.</p>' +
    '<form id="form">' + fields +
    '<button type="submit">Save</button></form>' +
    '<script>' +
    'document.getElementById("form").addEventListener("submit",function(e){' +
    'e.preventDefault();var out={};' +
    'Array.prototype.forEach.call(document.querySelectorAll("input"),function(i){' +
    'out[i.name]=i.value;});' +
    'var data=encodeURIComponent(JSON.stringify(out));' +
    // The emulator opens this page in a normal browser and passes return_to.
    'var m=/[?&]return_to=([^&]+)/.exec(location.search);' +
    'location.href=m?decodeURIComponent(m[1])+data:"pebblejs://close#"+data;' +
    '});' +
    '</script></body></html>'
  );
}

function buildUrl() {
  return 'data:text/html;charset=utf-8,' + encodeURIComponent(buildHtml(settings.read()));
}

// Saves the response from the settings page. Returns true when it was saved.
function save(response) {
  if (!response) {
    return false;
  }
  var values;
  try {
    values = JSON.parse(decodeURIComponent(response));
  } catch (e) {
    console.log('Could not read the settings response: ' + e);
    return false;
  }
  if (!values || typeof values !== 'object') {
    return false;
  }
  settings.write({
    baseUrl: settings.normalizeBaseUrl(values.baseUrl),
    token: (values.token || '').trim(),
    language: (values.language || '').trim(),
    pipeline: (values.pipeline || '').trim(),
  });
  return true;
}

module.exports = {
  buildUrl: buildUrl,
  save: save,
};
