# Resilience scenarios

The component is broken on purpose to check how it behaves. Each scenario is automated in
`aurorasync_light_player/test/test_integration.c`, tagged `[resilience]`, and runs with the other
tests in the `Unit tests` CI jobs (see [CONTRIBUTING.md](../../CONTRIBUTING.md#checks)).

Run on 2026-11-12 at `bdb9b45`, locally: ESP-IDF v6.1, QEMU 9.2.2 (esp32c3) and the linux target (GCC 16.2).

| Scenario                                                      | Targets     | Result      | Follow-up |
|---------------------------------------------------------------|-------------|-------------|-----------|
| [Malformed commands](#1-malformed-commands)                   | QEMU, linux | As expected | None      |
| [Timer cut off mid-animation](#2-timer-cut-off-mid-animation) | QEMU        | As expected | None      |

## 1. Malformed commands

Test: `random commands keep a valid state below full duty`.

### Setup

- The state comes from `light_control_init()`, with the master switched on: none of the 10,000 payloads
  is an accepted master command, so with the master off every level would stay at 0.
- 10,000 payloads from a xorshift32 generator, seed 35, so every target gets the same ones. Each payload has
  a length from 0 to 8 and random bytes. Half of the time, its first byte is replaced by one of the 8
  `LIGHT_CMD_*` opcodes: with fully random bytes, only about 26 payloads would be accepted.
- Each payload goes through `light_control_validate_command()`. An accepted one goes through
  `light_control_apply_command()` and `light_control_apply()`, then the levels are read back with
  `led_pwm_get_levels()`.

### Expected

- No crash.
- A rejected payload returns `-EMSGSIZE` or `-EINVAL` and leaves the encoded state unchanged.
- An accepted payload gives a valid `LIGHT_STATE`: version 1, flag bits 3 to 7 at 0.
- No level ever at `LED_PWM_LEVEL_MAX`.

### Actual

As expected on QEMU and linux: 439 payloads accepted, 9,561 rejected. QEMU log:

```
10000 payloads: 439 accepted, 9561 rejected
.../test/test_integration.c:49:random commands keep a valid state below full duty:PASS
Test ran in 16ms
```

The test can fail: with the full-scale clamp removed from `level_to_pwm()`, it stops on
`Expected 65535 to be less than 65535`.

## 2. Timer cut off mid-animation

Test: `sequence cut off for 1 s catches up with the clock and ends on its target`. QEMU only: there is no
`esp_timer` on linux.

### Setup

- A one-shot sequence of 3 linear segments of 1 s: red, rising from off to brightness 85, 170, then 255.
  The levels change at every step, so a step running during the cut would show.
- The light player plays it from t = 0. `light_player_cancel()` at t = 1.5 s, in the second segment,
  then `light_player_schedule()` at t = 2.5 s.

### Expected

- During the cut, no step runs and the levels do not change.
- On resume, the late step skips the rest of the second segment (`light_seq_catch_up()`). 100 ms later,
  the red level is above the start of the last segment: the sequence neither restarted nor resumed
  where it stopped.
- At t = 3.1 s, the sequence is over and the levels are those of its target: it caught up with the clock
  rather than ending 1 s late.

### Actual

As expected. The sequence started at 318 ms and ended at 3318 ms, so the 1 s cut did not delay it:

```
I (318) LightPlayer_LIGHT_SEQUENCE: Sequence started with 3 segments, loops=1
I (3318) LightPlayer_LIGHT_SEQUENCE: Sequence complete
.../test/test_integration.c:128:sequence cut off for 1 s catches up with the clock and ends on its target:PASS
Test ran in 3094ms
```

The test can fail:

- with `light_player_cancel()` doing nothing, steps keep running during the cut (`Expected 146 Was 245`);
- with `LIGHT_SEQ_MAX_CATCH_UP` at 1, the late step restarts the last segment instead of catching up,
  and the sequence is still running at t = 3.1 s.
