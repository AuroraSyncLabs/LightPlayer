# Contributing to AuroraSync Light Player

Thanks for taking the time to contribute! This guide explains how to report issues,
propose changes and get them merged.

This repository holds a single ESP-IDF component, published on the
[ESP Component Registry](https://components.espressif.com/) as `anabolicroo/aurorasync_light_player`:

| Directory                           | Content                                        |
|-------------------------------------|------------------------------------------------|
| `aurorasync_light_player/src/`      | Component sources                              |
| `aurorasync_light_player/include/`  | Public headers (`lightplayer/*.h`)             |
| `aurorasync_light_player/examples/` | Example projects using the component           |
| `aurorasync_light_player/Kconfig`   | Component configuration (`LIGHT_PLAYER_*`)     |

Toolchain: ESP-IDF >= 5.3 (the maintainers use ESP-IDF v6.1).

## Reporting a bug

Open an issue using the **Bug report** template (`.github/ISSUE_TEMPLATE/bug_report.md`).
The title is prefixed with `[BUG]` and the `bug` label is applied automatically.

Please fill in every section of the template:

- **Describe the bug**: a clear and concise description of the problem.
- **To Reproduce**: numbered steps to reproduce the behavior.
- **Expected behavior**: what you expected to happen instead.
- **Screenshots/Proofs**: screenshots, oscilloscope captures, videos or logs, if applicable.
- **Used devices**: the dev board (e.g. ESP32-S31, ESP32-C6). Please also give the
  component version and the ESP-IDF version you build with.
- **Additional context**: anything else that helps, such as your `LIGHT_PLAYER_*` Kconfig values.

Before opening a new issue, search the existing ones to avoid duplicates.

## Requesting a feature

Open an issue using the **Feature request** template (`.github/ISSUE_TEMPLATE/feature_request.md`).
The title is prefixed with `[FEATURE]` and the `enhancement` label is applied automatically.
Describe the problem, the solution you would like and the alternatives you considered.

## Scope of contributions

| Area                                                                                   | Status                                                                       |
|----------------------------------------------------------------------------------------|------------------------------------------------------------------------------|
| Docs, examples, tests, error messages, bug fixes, new easing curves or targets         | Open                                                                         |
| Public API (`include/lightplayer/`), Kconfig options                                   | Discuss in an issue first: changes break dependent projects                  |
| Payload format (`LIGHT_CMD_*`, `LIGHT_STATE_*`), mixing policies, calibration defaults | Maintainers only: tied to AuroraSync's apps and LEDs, but discussion is open |

A pull request is accepted when it is linked to an issue, does one thing, keeps every example building,
was tested on hardware if it changes the light output, and updates the docs if it changes behavior.

It is declined, always with a written reason, when it breaks the API or payload format without agreement,
adds a non-ESP-IDF dependency, or mixes unrelated changes.

We answer issues and pull requests within **7 days**. No answer? Mention a [maintainer](README.md#maintainers).

## Working on an issue

Every change should be linked to an issue. When you pick one up:

1. Assign yourself to it.
2. Set its **size** (see below).
3. Create a branch from `main`.

### Issue size

| Size | Estimated effort |
|------|------------------|
| XS   | < 1 day          |
| S    | 1 day            |
| M    | 2–3 days         |
| L    | 4–7 days         |
| XL   | > 7 days         |

If an issue is XL, consider splitting it into smaller ones.

## Branches

- `main`: released and releasable code. All feature branches start from and merge back into `main`.
- Feature branches are named after their issue, using the branch name GitHub generates
  from the issue:

  ```
  feature/<issue-number>-<short-description>
  ```

  For example: `feature/12-add-easing-functions`.

## Commit messages

We follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <short summary in imperative mood>
```

- **Types**: `feat`, `fix`, `refactor`, `test`, `docs`, `style`, `perf`, `chore`.
- **Scopes**: the area of the component (`easing`, `led-pwm`, `light-control`, `sequence`,
  `player`, `kconfig`), `examples`, `cicd` or `readme`.
- Mark breaking changes with `!` after the scope, e.g. `feat(player)!: take the sequence by pointer`.
  This applies in particular to any change to the public headers in `include/lightplayer/`
  or to the Kconfig options, since it breaks the projects depending on the component.

Examples:

```
feat(easing): add ease-in-out cubic curve
fix(led-pwm): clamp duty cycle to the LEDC resolution
docs(readme): update installation instructions
```

## Building and testing

Source the ESP-IDF environment first, e.g. with fish:

```shell
. /path/to/.espressif/v6.1/esp-idf/export.fish
```

The component is built through its examples, which point to the local sources
with `override_path`. From an example directory, e.g. `aurorasync_light_player/examples/basic_blink/`:

```shell
idf.py set-target esp32s31
idf.py build
idf.py flash monitor
```

Make sure every example still builds when you change the component, and add or update
an example when you add a feature.

### Checks

CI runs on every pull request (`.github/workflows/ci.yml`): formatting, `idf.py clang-check`,
unit tests on QEMU and the example builds. From `aurorasync_light_player/`:

```shell
# Formatting, pinned version through uv (https://docs.astral.sh/uv/)
git ls-files '*.c' '*.h' | xargs uvx clang-format==23.1.1 --style=file -i
# Static analysis (needs `idf_tools.py install esp-clang`), findings in warnings.txt
cd examples/rampup_hold_rampdown && IDF_TOOLCHAIN=clang idf.py set-target esp32c6 \
  && idf.py clang-check --include-paths "$(realpath ../..)" \
     --run-clang-tidy-options "-header-filter=aurorasync_light_player/"
```

Unit tests live in `test/test_<module>.c` (`TEST_CASE("<behavior>", "[<module>]")`) and run
through `test_apps/` with [pytest](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/contribute/esp-idf-tests-with-pytest.html)
(`install.sh --enable-ci`, plus `idf_tools.py install qemu-riscv32` for QEMU). A bug fix comes with a test.

```shell
# QEMU, as in CI
idf-ci build run -p test_apps -t esp32c3 -m qemu && pytest -m qemu --target esp32c3
# Board
idf-ci build run -p test_apps -t esp32c6 -m generic && pytest -m generic --target esp32c6
```

## Pull requests

1. Open the pull request against `main`.
2. Fill in the pull request template:
   - a description of the change,
   - the related issue (`Fixes #<issue-number>`, so it closes automatically),
   - the type of change,
   - how it was tested, with reproduction steps and the boards used,
   - the checklist,
   - screenshots, captures or a demo for any visible light behavior change.
3. Get at least one review from another team member and address the feedback.

## Definition of Done

An issue is done when:

- **Functionality implemented**: the requirement of the issue works as specified.
- **Code written**: clean, follows the coding standards, commented where needed.
- **Unit tested**: new code has unit tests where relevant, and all tests pass.
- **Integration tested**: the examples build and behave as expected on real hardware.
- **Peer reviewed**: reviewed by at least one other team member, feedback addressed.
- **Manually tested**: by the developer, and preferably by another team member.
- **Documentation complete**: public API changes, Kconfig options, technical decisions and
  architecture choices are documented (headers, README, examples).
  For research tasks, findings, comparisons and final choices are documented with justifications.
- **Issue updated & closed**: the issue has relevant comments and links to the pull request, then is closed.

## Releases

Releases are made by maintainers:

1. Bump `version` in `aurorasync_light_player/idf_component.yml`, following
   [Semantic Versioning](https://semver.org/) (a breaking change bumps the major version).
2. Push a tag `vX.Y.Z` matching that version.

The upload component workflow then publishes the component to the ESP Component Registry.

## Questions

If something is unclear, open an issue or reach out to one of the [maintainers](README.md#maintainers).
