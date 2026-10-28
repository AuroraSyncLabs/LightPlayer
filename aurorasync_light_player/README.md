# AuroraSync Light Player Component

ESP-IDF component driving a 5-channel PWM LED fixture with eased light animations.
See the [repository README](https://github.com/AuroraSyncLabs/LightPlayer) for its scope.

## Hardware

5 LED channels, one GPIO each through LEDC: RGB strip red, green and blue, FlameWarm, SkyBlue.
The [examples](examples/) show the default wiring.

## How to use the component

### Pre-requisites

- IDF: A version >=6.1 of IDF SDK is required, in order to use this component into your project.
  - Installation instructions: https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/get-started/index.html#installation
- An already existing project.

#### Notes

- The project has been tried on the ESP32-S31 and ESP32-C6.

### Add the dependency to your project

1. Navigate into your project.
    ```bash
    cd path/to/your_project
    ```
2. Source the SDK environment.
    ```bash
    # Bash, ZSH
    . /path/to/.espressif/v6.1/esp-idf/export.sh
    # Fish
    . /path/to/.espressif/v6.1/esp-idf/export.fish
    ```
3. Add the dependency.
    ```bash
    idf.py add-dependency aurorasynclabs/aurorasync_light_player
    ```
4. Edit LIGHT_PLAYER_PWM_GPIO_* values for your usage.
5. Build and flash.
    ```bash
    idf.py build flash
    ```

`--preview` is only needed for targets ESP-IDF still lists as preview, such as the ESP32-S31.

### Run on the linux target

The component also builds for the ESP-IDF `linux` target, to run your application on the host.

- The PWM outputs are emulated: no GPIO is driven and the `PWM (LEDC) outputs` menu is hidden.
  Read the applied levels back with `led_pwm_get_levels()`.
- ESP-IDF only ships the `esp_timer` headers on linux. If your application uses `light_player_*`,
  it must provide `esp_timer_create()`, `esp_timer_start_once()`, `esp_timer_stop()` and
  `esp_timer_get_time()`, or use the ESP-IDF mock (`$IDF_PATH/tools/mocks/esp_timer`) in unit tests.

```bash
idf.py --preview set-target linux
idf.py build
./build/your_project.elf
```

### Try the component

1. Clone the project.
    ```bash
    git clone https://github.com/AuroraSyncLabs/LightPlayer.git
    ```
2. Navigate into an example (and read the README.md for more details about PWM configuration).
    ```bash
    cd LightPlayer/aurorasync_light_player/examples/basic_blink
    ```
3. Source the SDK environment.
    ```bash
    # Bash, ZSH
    . /path/to/.espressif/v6.1/esp-idf/export.sh
    # Fish
    . /path/to/.espressif/v6.1/esp-idf/export.fish
    ```
4. Set the target, e.g. ESP32-C6, or ESP32-S31 with `--preview`.
    ```bash
    idf.py --preview set-target esp32s31
    ```
5. Build, flash and watch the logs.
    ```bash
    # Build
    idf.py --preview build

    # Flash and monitor (with your board connected via USB)
    idf.py --preview -p PORT flash monitor
    ```

# Maintainers

<div align="center">

| [<img src="https://github.com/AnabolicRoo.png" width="85"><br>**Julien ROIRON**](https://github.com/AnabolicRoo) | [<img src="https://github.com/PasVegan.png" width="85"><br>**Quentin FILLIAERT**](https://github.com/PasVegan) | [<img src="https://github.com/ValentinRapp.png" width="85"><br>**Valentin RAPP**](https://github.com/ValentinRapp) |
|:----------------------------------------------------------------------------------------------------------------:|:--------------------------------------------------------------------------------------------------------------:|:------------------------------------------------------------------------------------------------------------------:|

</div>