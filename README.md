# AuroraSync Light Player Component

ESP-IDF component driving a 5-channel PWM LED fixture (RGB strip, FlameWarm, SkyBlue) with eased light
animations. Published on the [ESP Component Registry](https://components.espressif.com/components/anabolicroo/aurorasync_light_player).

## Features

- 5 LEDC PWM outputs. Pins, frequency and gains set in `idf.py menuconfig`.
- One color for the whole fixture, or each strip set independently.
- Animations of up to 4 eased segments, looped N times or forever, stoppable at any time.
- Payload helpers to control the light over BLE or any other transport.

## Out of scope

- Communication stack: sending and receiving payloads is up to your application.
- Addressable LEDs, other channel layouts, several players at once.
- Frameworks other than ESP-IDF >= 5.3.

## Demonstration of examples

https://github.com/user-attachments/assets/915d31ee-7af1-4be0-b5a7-915fb8bea987
> Simple blink example

https://github.com/user-attachments/assets/5c5981b6-0fcc-4a8b-8e39-033ddf675beb
> Ramp-up hold and ramp-down example

## Quick start

```bash
idf.py add-dependency anabolicroo/aurorasync_light_player
```

| Documentation                  | Where                                                                                                                            |
|--------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| Installation and configuration | [`aurorasync_light_player/README.md`](aurorasync_light_player/README.md)                                                         |
| Examples, wiring and demo      | [`aurorasync_light_player/examples/`](aurorasync_light_player/examples/)                                                         |
| API reference                  | [`aurorasync_light_player/include/lightplayer/`](aurorasync_light_player/include/lightplayer/)                                   |
| Contributing                   | [`CONTRIBUTING.md`](CONTRIBUTING.md)                                                                                             |
| Tasks for newcomers            | [`good first issue`](https://github.com/AuroraSyncLabs/LightPlayer/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22) |

## Maintainers

<div align="center">

| [<img src="https://github.com/AnabolicRoo.png" width="85"><br>**Julien ROIRON**](https://github.com/AnabolicRoo) | [<img src="https://github.com/PasVegan.png" width="85"><br>**Quentin FILLIAERT**](https://github.com/PasVegan) | [<img src="https://github.com/ValentinRapp.png" width="85"><br>**Valentin RAPP**](https://github.com/ValentinRapp) |
|:----------------------------------------------------------------------------------------------------------------:|:--------------------------------------------------------------------------------------------------------------:|:------------------------------------------------------------------------------------------------------------------:|

</div>