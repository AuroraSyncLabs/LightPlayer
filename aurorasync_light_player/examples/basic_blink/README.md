| Tested Targets | ESP32-S31 |
|----------------|-----------|

# Basic Blink Example

Blinks the whole light fixture in white: 1 second on at full brightness, 1 second off, forever.

This is the smallest possible use of the AuroraSync Light Player component. It shows the required
initialization order and how to drive the light directly through the logical light state, without
any animation.

## Demo

https://github.com/user-attachments/assets/915d31ee-7af1-4be0-b5a7-915fb8bea987

## What it shows

1. `led_pwm_init()` sets up the LEDC PWM outputs. It must be called first, otherwise every other
   call fails with `-EAGAIN`.
2. `light_control_init()` fills a `light_state_t` with the defaults (master off, global mode, full
   white) and applies it.
3. `light_control_set_master()` and `light_control_set_global()` change the logical state.
4. `light_control_apply()` renders that state and writes it to the PWM outputs. Nothing changes on
   the LEDs until it is called.

## Hardware

Connect the LED channels to the following GPIOs (Kconfig defaults):

| Channel         | GPIO |
|-----------------|------|
| RGB strip red   | 14   |
| RGB strip green | 15   |
| RGB strip blue  | 16   |
| FlameWarm       | 17   |
| SkyBlue         | 19   |

The pins, the PWM frequency and the per-channel gains can be changed in
`idf.py menuconfig` -> `Light player`.

## How to use

```bash
idf.py --preview set-target esp32s31
idf.py --preview build flash monitor
```

## Expected output

```
Basic blink Exemple!
Keep it on for 1 sec.
Keep it off for 1 sec.
Keep it on for 1 sec.
...
```
