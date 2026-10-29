# Appstore listing: HA Voice

Everything the Rebble developer portal asks for, ready to paste or upload.
Portal: <https://dev-portal.rebble.io/> (needs a Rebble account; the app's author
name comes from your developer profile).

## Fields

| Portal field | Value |
| --- | --- |
| App name | `HA Voice` |
| Type | Watch App (not a watchface) |
| Category | Tools & Utilities |
| Website | your choice; the project has no page yet, so leave it out or point it at a source repository |
| Source code | optional; add the repository address if you publish it |
| Visibility | public |

## Short description (one line, for lists and search results)

> Ask Home Assistant from your wrist: say a command, read the answer.

## Description

> Talk to Home Assistant from your wrist. Tap the microphone, say what you
> want, and read the answer on the watch.
>
> HA Voice sends what you say to your own Home Assistant server, through your
> phone, using the Assist conversation API. Ask for the temperature, turn
> lights on and off, or run any intent your Assist setup already answers.
>
> What you need:
>
> - Home Assistant reachable from your phone, with Assist set up
> - A long-lived access token from your Home Assistant profile
> - The Pebble app on the phone, connected to the watch
>
> Setup: open the watch menu, choose Settings, and enter your Home Assistant
> address, such as http://192.168.1.10:8123, and the token. Then choose Test
> connection in the watch menu to check both.
>
> Notes:
>
> - The answer is text only. The watch does not read it out loud.
> - Dictation is done by the Pebble app and your phone's speech service, as in
>   any other Pebble app. Only the transcribed text goes to your Home Assistant
>   server; nothing goes anywhere else.
> - Works on Pebble Time 2, Pebble Time, Pebble Time Steel and Pebble 2. All of
>   them have a microphone.
> - If your Home Assistant uses a self-signed certificate over https, use the
>   http address instead.

## Release notes, version 1.0.1

> First public version.
>
> - Tap the microphone on the main screen, or press SELECT, to ask a question.
> - The answer scrolls if it is longer than the screen; ASK AGAIN at the bottom
>   asks the next question.
> - Swipe down, or press DOWN, for the menu: ask again, settings, and a
>   connection test.
> - Long answers arrive in pieces, so they are not cut short.
> - The watch buzzes when an answer arrives.

## Images

| What | File | Size |
| --- | --- | --- |
| Icon | `icon-144.png` | 144 × 144 |
| Banner | `banner-720x320.png` | 720 × 320 |
| Screenshot, main screen | `screenshots/emery-main.png` | 200 × 228 |
| Screenshot, answer | `screenshots/emery-answer.png` | 200 × 228 |
| Screenshot, menu | `screenshots/emery-menu.png` | 200 × 228 |
| Screenshot, connection test | `screenshots/emery-test.png` | 200 × 228 |

The screenshots are the `emery` platform, which is the Pebble Time 2. The portal
keeps a separate screenshot set for each platform, so the app also needs a set
at 144 × 168 for `basalt` and `flint` if those are listed.

Rebuild the images after an icon or screen change:

    .venv/bin/python scripts/make-icons.py
    .venv/bin/python scripts/make-store-assets.py

## The upload

1. Sign in at <https://dev-portal.rebble.io/>.
2. Upload `build/pt2-voice-assistant.pbw` from the release build.
3. Fill in the fields above, upload the icon, the banner and the screenshots.
4. Submit. The app appears in the appstore after review.

Once the app is in the appstore, the phone app shows this icon for it: the
phone app draws an app's icon from its store listing, not from the app bundle,
which is why a sideloaded copy shows a placeholder.
