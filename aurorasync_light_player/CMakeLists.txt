idf_component_register(
    SRCS
        "src/easing.c"
        "src/led_pwm.c"
        "src/light_control.c"
        "src/light_sequence.c"
        "src/light_player.c"
        "src/port/esp/led_pwm_port.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src/port"
    REQUIRES esp_driver_ledc esp_timer)
