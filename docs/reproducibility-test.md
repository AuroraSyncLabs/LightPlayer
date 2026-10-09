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

## Report: TODO date

| Field      | Value                                         |
|------------|-----------------------------------------------|
| Tester     | TODO name, school or company, not on the team |
| Experience | TODO e.g. C yes, ESP-IDF never                |
| Machine    | TODO OS and version, clean install or VM      |
| Board      | TODO e.g. ESP32-C6-DevKitC-1                  |
| Total time | TODO                                          |
| Result     | TODO LEDs blink / failed at step N            |

### Steps

| Step                     | Time | Blocker or question | Help given |
|--------------------------|------|---------------------|------------|
| Install ESP-IDF          | TODO | TODO                | TODO       |
| Clone the repository     | TODO | TODO                | TODO       |
| Source the environment   | TODO | TODO                | TODO       |
| Set the target           | TODO | TODO                | TODO       |
| Build, flash and monitor | TODO | TODO                | TODO       |
| Change the LED GPIOs     | TODO | TODO                | TODO       |

### Fixes made to the documentation

| Blocker | Fix  | Commit |
|---------|------|--------|
| TODO    | TODO | TODO   |
