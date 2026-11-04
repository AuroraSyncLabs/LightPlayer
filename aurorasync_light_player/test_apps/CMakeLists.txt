# Unit test app: runs the test cases of ../test with the Unity menu.
cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS ../test)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(light_player_test)

if(IDF_TARGET STREQUAL "linux")
    idf_component_get_property(lib aurorasync_light_player COMPONENT_LIB)
    target_compile_options(${lib} PRIVATE --coverage)
    target_link_options(${CMAKE_PROJECT_NAME}.elf PRIVATE --coverage)
endif()
