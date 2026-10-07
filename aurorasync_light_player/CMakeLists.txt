idf_component_register(
    SRCS
        "src/easing.c"
        "src/led_pwm.c"
        "src/light_control.c"
        "src/light_sequence.c"
        "src/light_player.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_ledc esp_timer)
