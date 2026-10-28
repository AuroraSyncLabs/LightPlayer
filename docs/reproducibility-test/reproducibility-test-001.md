# Reproducibility test

Someone outside the team installs the project on a clean machine, using only the documentation.
One report per test. Fill in every `TODO` during the test, not afterwards.

## Protocol

1. The tester starts from a machine without ESP-IDF, or a fresh user account or virtual machine.
2. They get only the link to the repository README. No help from the team, except to unblock them
   after 15 minutes stuck; write down every time you help.
3. Goal: run the `basic_blink` example on a board, following
   [Try the component](../aurorasync_light_player/README.md#try-the-component).
4. Time each step. Write down every question, error and wrong guess.
5. Fix the documentation for each blocker and link the commit below.

## Report: 2026-10-28

| Field      | Value                                         |
|------------|-----------------------------------------------|
| Tester     | Hugo Payet, EPITECH Paris (not on the team)   |
| Experience | C yes, ESP-IDF never                          |
| Machine    | Windows 10 Pro (build 19045), clean machine   |
| Board      | ESP32-C6-DevKitC-1                            |
| Total time | 15 min                                        |
| Result     | LEDs blink                                    |

### Steps

| Step                     | Time  | Blocker or question                                                                                     | Help given |
|--------------------------|-------|---------------------------------------------------------------------------------------------------------|------------|
| Install ESP-IDF          | 6 min | None (installed via winget using Espressif Installation Manager).                                       | None       |
| Clone the repository     | 1 min | None (repository was cloned locally).                                                                   | None       |
| Source the environment   | 2 min | README only documented Unix shells (`export.sh`, `export.fish`); needed Windows PowerShell/CMD scripts. | None       |
| Set the target           | 1 min | None (set target to esp32c6 with `idf.py set-target esp32c6`).                                          | None       |
| Build, flash and monitor | 3 min | Documentation bundled `build flash monitor` together; separated build before flashing.                  | None       |
| Change the LED GPIOs     | 2 min | None (configured in `idf.py menuconfig` -> `Light player`, LEDs blink with new pins).                   | None       |

### Fixes made to the documentation

| Blocker                                            | Fix                                                                                                | Commit                                                              |
|----------------------------------------------------|----------------------------------------------------------------------------------------------------|---------------------------------------------------------------------|
| Missing Windows instructions to source environment | Document PowerShell (`. export.ps1`) and CMD (`export.bat`) commands in the component README       | [`16d5221`](https://github.com/AuroraSyncLabs/LightPlayer/pull/40) |
| Bundled `build flash monitor` in README snippet    | Document `idf.py build` separately from flashing and monitoring (`idf.py -p PORT flash monitor`)   | [`c492e84`](https://github.com/AuroraSyncLabs/LightPlayer/pull/41) |
