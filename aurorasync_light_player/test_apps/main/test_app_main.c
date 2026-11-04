/*
** Julien ROIRON, 2026
** test_app_main.c
** File description:
** Entry point of the unit test app
*/

#include <sdkconfig.h>
#include <unity.h>

#if CONFIG_IDF_TARGET_LINUX
#include <gcov.h>
#endif

void tearDown(void)
{
#if CONFIG_IDF_TARGET_LINUX
    /* pytest-embedded kills the app, so the exit hook never writes the coverage counters. */
    __gcov_dump();
    __gcov_reset();
#endif
}

void app_main(void)
{
    unity_run_menu();
}
