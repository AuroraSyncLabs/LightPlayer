| Tested Targets | ESP32-S31 |
|----------------|-----------|

# Ramp-up Hold Ramp-down Example

Plays a 3-second animation once, then restarts the chip and plays it again:

| Segment   | Duration | Curve          | Light                |
|-----------|----------|----------------|----------------------|
| Ramp-up   | 1000 ms  | `EASE_SMOOTH`  | Off to full red      |
| Hold      | 1000 ms  | `EASE_LINEAR`  | Stays full red       |
| Ramp-down | 1000 ms  | `EASE_RELEASE` | Full red back to off |

## Demo

https://github.com/user-attachments/assets/5c5981b6-0fcc-4a8b-8e39-033ddf675beb

## What it shows

1. `led_pwm_init()` sets up the LEDC PWM outputs, then `light_control_init()` builds the two light
   states the sequence moves between (off and full red).
2. `light_player_module_init()` creates the timer driving the player. Its step callback simply
   forwards to `light_seq_step()`, which renders the current frame and writes it to the LEDs.
3. A `light_seq_t` describes the animation: up to `LIGHT_SEQ_MAX_SEGMENTS` segments, each with a
   target state, a duration and an easing curve, played `loops` times (`0` means forever).
4. `light_seq_start()` loads the sequence, and `light_player_schedule()` runs the first step. The
   player then steps itself until the sequence is over.

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
Ramp-up Hold Ramp-down Example!
Wait 5000ms until it finishes.
Restarting now.
```
