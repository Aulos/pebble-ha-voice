// Phone-side entry point for HA Voice.
//
// Two jobs: bring up the Home Assistant bridge (ha.js) and own the settings page
// that collects the Home Assistant address and long-lived token.

var ha = require('./ha');
var settingsPage = require('./settings-page');

ha.init();

Pebble.addEventListener('ready', function () {
  ha.sendConfigState();
});

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL(settingsPage.buildUrl());
});

Pebble.addEventListener('webviewclosed', function (event) {
  if (settingsPage.save(event && event.response)) {
    ha.sendConfigState();
  }
});
