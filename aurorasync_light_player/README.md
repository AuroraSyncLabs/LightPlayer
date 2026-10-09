# AuroraSync Light Player Component

ESP-IDF component driving a 5-channel PWM LED fixture with eased light animations.
See the [repository README](https://github.com/AuroraSyncLabs/LightPlayer) for its scope.

## Hardware

5 LED channels, one GPIO each through LEDC: RGB strip red, green and blue, FlameWarm, SkyBlue.
The [examples](examples/) show the default wiring.

## How to use the component

### Pre-requisites

- IDF: A version >=5.3 of IDF SDK is required, in order to use this component into your project. 
  - Installation instructions: https://docs.espressif.com/projects/esp-idf/en/v5.3.6/esp32/get-started/index.html#installation
- An already existing project.

#### Notes

- The project has been tried on the ESP32-S31 and ESP32-C6.
- The SDK version used by the maintainers is IDF 6.1.

### Add the dependency to your project

1. Navigate into your project.
    ```bash
    cd path/to/your_project
    ```
2. Source the SDK environment (fish command).
    ```bash
    . /path/to/.espressif/v6.1/esp-idf/export.fish
    ```
3. Add the dependency.
    ```bash
    idf.py add-dependency anabolicroo/aurorasync_light_player
    ```
4. Edit LIGHT_PLAYER_PWM_GPIO_* values for your usage.
5. Build and flash.
    ```bash
    idf.py --preview build flash
    ```

### Try the component

1. Clone the project.
    ```bash
    git clone https://github.com/AuroraSyncLabs/LightPlayer.git
    ```
2. Navigate into an example.
    ```bash
    cd LightPlayer/aurorasync_light_player/examples/basic_blink
    ```
3. Source the SDK environment (fish command).
    ```bash
    . /path/to/.espressif/v6.1/esp-idf/export.fish
    ```
4. Set target.
    ```bash
    idf.py --preview set-target esp32s31
    ```
5. Build and flash.
    ```bash
    idf.py --preview build flash
    ```

# Maintainers

<div align="center">

| [<img src="https://github.com/AnabolicRoo.png" width="85"><br>**Julien ROIRON**](https://github.com/AnabolicRoo) | [<img src="https://github.com/PasVegan.png" width="85"><br>**Quentin FILLIAERT**](https://github.com/PasVegan) | [<img src="https://github.com/ValentinRapp.png" width="85"><br>**Valentin RAPP**](https://github.com/ValentinRapp) |
|:----------------------------------------------------------------------------------------------------------------:|:--------------------------------------------------------------------------------------------------------------:|:------------------------------------------------------------------------------------------------------------------:|

</div>