idf_build_get_property(target IDF_TARGET)

set(srcs
    "src/easing.c"
    "src/led_pwm.c"
    "src/light_control.c"
    "src/light_sequence.c"
    "src/light_player.c")
set(requires esp_timer)

if(target STREQUAL "linux")
    list(APPEND srcs "src/port/linux/led_pwm_port.c")
else()
    list(APPEND srcs "src/port/esp/led_pwm_port.c")
    list(APPEND requires esp_driver_ledc)
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src/port"
    REQUIRES ${requires})
