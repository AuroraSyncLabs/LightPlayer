# Scope of the opening

AuroraSync is a commercial smart light: a 5-channel LED fixture, its firmware and a mobile app.
We open one component of it, the light player, and keep the rest closed.

## Open: this repository

The `aurorasync_light_player` ESP-IDF component, with its examples, tests and CI:

- 5-channel LEDC PWM driver, with an emulated backend for the ESP-IDF `linux` target,
- light state and color mixing (`light_control`),
- eased animation sequences and their player (`easing`, `light_sequence`, `light_player`),
- payload helpers to encode and decode light commands and states.

## Closed

| Part                 | Content                                                      |
|----------------------|--------------------------------------------------------------|
| Application firmware | BLE service, pairing and encryption, alarms, OTA integration |
| Mobile app           | Android app controlling the light                            |
| Hardware             | PCB module and LED selection                                 |

## Why

The closed parts are the product: the user experience, the behavior of the light and the device
people buy. Opening them would give the product away.

The light player is the reusable part:

- **Bounded**: it only depends on ESP-IDF. No BLE, app or AuroraSync code is needed to use it.
- **Reusable**: anyone driving PWM LED strips from an ESP32 can use it. Pins, frequency and gains are
  set in `idf.py menuconfig`, without changing the code.
- **Separated**: it has its own repository and is published on the
  [ESP Component Registry](https://components.espressif.com/components/aurorasynclabs/aurorasync_light_player).

Opening it costs the product nothing. The payload format is public, but sending and receiving it
(the BLE service, pairing and encryption) stays in the closed firmware.

## Limits

- The 5-channel layout mirrors the AuroraSync fixture. Other layouts need code changes and are out of scope
  (see the [README](../README.md#out-of-scope)).
- The payload format and the calibration defaults stay under maintainer control, since the AuroraSync app
  depends on them (see [Scope of contributions](../CONTRIBUTING.md#scope-of-contributions)).
