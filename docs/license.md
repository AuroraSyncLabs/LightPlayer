# License

The component is licensed under the [Mozilla Public License 2.0](../LICENSE) (MPL-2.0).

## Why MPL-2.0

| License         | Effect on the component                                                                     | Verdict  |
|-----------------|---------------------------------------------------------------------------------------------|----------|
| GPL             | Any firmware linking the component must be GPL, including AuroraSync's closed firmware      | Rejected |
| MIT, Apache-2.0 | Anyone can ship a modified copy of the component closed, and fixes never come back          | Rejected |
| MPL-2.0         | Copyleft on our files only: changes to the component stay open, the code around it does not | Chosen   |

Contributions arrive under MPL-2.0 too, so AuroraSync keeps using the component, contributions included,
in its closed firmware. Under the GPL it could not, without the agreement of every contributor.

## What it allows and forbids

| You want to                           | Allowed | Conditions                                                                                             |
|---------------------------------------|---------|--------------------------------------------------------------------------------------------------------|
| Use the component in your project     | Yes     | Any license for your own code. When you ship a binary, say where the component source is (§3.2)        |
| Modify it                             | Yes     | Modified files stay MPL-2.0. If you distribute them, in source or binary, publish their source         |
| Add your own files next to it         | Yes     | Files holding none of our code can use any license (§1.10, §3.3)                                       |
| Ship it in a commercial product       | Yes     | Your product code stays closed. Only the component files must be available as source                   |
| Combine it with GPL code              | Yes     | No Exhibit B notice, so it can be distributed under the GPL inside a GPL project (§3.3)                |
| Use the AuroraSync name or logo       | No      | No trademark rights are granted (§2.3)                                                                 |
| Keep your license after a patent suit | No      | Each contributor grants their patents (§2.1); suing over a patent in the code ends your license (§5.2) |

The component comes with no warranty and no liability (§6, §7).

## License file

- `LICENSE` and `aurorasync_light_player/LICENSE` hold the full, unmodified MPL-2.0 text. The second one ships
  in the registry package.
- `aurorasync_light_player/idf_component.yml` declares `license: "MPL-2.0"`, and GitHub detects it.
- Source files carry no license notice of their own: as Exhibit A allows, the notice is in the `LICENSE` file
  of the component.

## Dependency check

Checked against ESP-IDF v6.1 on 2026-10-22.

| Dependency                                                  | Used by                                           | License                        | Compatible |
|-------------------------------------------------------------|---------------------------------------------------|--------------------------------|------------|
| ESP-IDF `esp_timer`, `esp_driver_ledc`, `log`, `esp_common` | `light_player.c`, `port/esp/led_pwm_port.c`, logs | Apache-2.0                     | Yes        |
| FreeRTOS kernel (ESP-IDF)                                   | `led_pwm.c`                                       | MIT                            | Yes        |
| picolibc (default with GCC), newlib (with clang)            | Every file (C library)                            | BSD-style                      | Yes        |
| Unity (ESP-IDF `unity`)                                     | `test/` only                                      | MIT                            | Yes        |
| pytest, pytest-embedded                                     | Test runner                                       | MIT                            | Not linked |
| idf-ci                                                      | Test runner                                       | Apache-2.0                     | Not linked |
| clang-format, clang-tidy                                    | CI                                                | Apache-2.0 WITH LLVM-exception | Not linked |

picolibc also ships GPL and AGPL files, but only a test case and a build script: they are not part of the library.

MPL-2.0 is compatible with Apache-2.0, MIT and BSD licenses: their code can sit in the same firmware, each
file keeping its own license and notices. No dependency imposes its license on the component or on the
firmware using it.

## Ownership

- Each author keeps the copyright of the code they wrote: Julien ROIRON and Quentin FILLIAERT, as named in the
  file headers. Files without a header are traced through the git history.
- There is no contributor license agreement and no copyright assignment. Contributors keep their copyright
  and license their contributions under MPL-2.0, the license of the files they change.
- Relicensing the component would need the agreement of every copyright holder. MPL-2.0 still lets the code
  move to a later MPL version (§10.2).
- The maintainers hold the `AuroraSyncLabs` GitHub organization and the `aurorasynclabs` namespace on the
  ESP Component Registry.
