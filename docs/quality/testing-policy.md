# Testing policy

## Critical path

The product is unusable if a command does not reach the LEDs, or if an animation does not play.
Two flows, 14 public functions:

| Flow | Functions |
|---|---|
| Command to light | `light_control_validate_command`, `light_control_apply_command`, `light_control_encode_state`, `light_control_render`, `light_control_apply_frame`, `led_pwm_init`, `led_pwm_set_levels` |
| Animation | `light_seq_start`, `light_seq_step`, `light_seq_request_stop`, `ease_apply`, `light_player_module_init`, `light_player_schedule`, `light_player_cancel` |

Testing effort goes there first. The other 17 public functions are setters and helpers built on them.

## Strategy

| Part | Test type | Automated |
|---|---|---|
| `easing.c`, `light_control.c`, `light_sequence.c`: pure logic | Unit tests (Unity `TEST_CASE`) | Yes, QEMU and linux in CI |
| `light_player.c` (esp_timer) with `light_sequence.c` and `led_pwm.c` | Integration test: init, start, schedule, wait, check the final state | Yes, QEMU in CI (no `esp_timer` implementation on linux) |
| `led_pwm.c`: common API | Unit tests, levels read back with `led_pwm_get_levels()` | Yes, QEMU and linux in CI |
| `port/esp/led_pwm_port.c` (LEDC driver) | Return codes in unit and integration tests | Yes, QEMU in CI |
| `port/linux/led_pwm_port.c` (emulated outputs) | Through the `led_pwm.c` unit tests | Yes, linux in CI |
| Light output: colors, smoothness, flicker | Manual, on an ESP32-C6 board with the examples, before each release | No: needs eyes or an oscilloscope |
| Examples | Build for IDF v6.1 | Yes, CI |

## Coverage target

- **Critical path: 100% of the 14 functions**, each with a nominal case and at least one error or edge case.
- **Whole public API: 80% (25 of 31 functions).** The remaining setters only copy fields and are covered through the tests above.

Coverage is counted per function, from the test list. Line coverage (gcov) needs JTAG on a real board
in ESP-IDF and does not run on QEMU. On the linux target the test app is a host executable, so gcov
could run in CI: not set up yet.

Status on 2026-10-20: 4 of 14 critical functions tested (`light_seq_start`, `light_seq_step`, `light_seq_request_stop`,
`led_pwm_set_levels`).

## Conventions

- Tests live in `aurorasync_light_player/test/test_<module>.c`, run by `test_apps/` (see `CONTRIBUTING.md`).
- Names: `TEST_CASE("<expected behavior>", "[<module>]")`; integration tests also get the `[integration]` tag.
- A new public function comes with its tests in the same pull request.
- A bug fix starts with a test that fails before the fix; the case is logged as a regression.
- Every test runs on QEMU and linux. A test calling `light_player_*` (`esp_timer`) or checking the LEDC
  driver is guarded with `#if !CONFIG_IDF_TARGET_LINUX`, after `#include <sdkconfig.h>`.
- CI runs every test on each pull request; a red test blocks the merge.
