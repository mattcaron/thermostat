/**
 * @file
 * Various temperature console commands.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "console.h"
#include "temperature.h"
#include "config_storage.h"

// static const char *TAG = "cmd_temperature";

#define TEMPERATURE_HELP_COMMAND "help"
#define TEMPERATURE_READ_COMMAND "read"

static void register_tasks(void);

void register_temperature(void)
{
    register_tasks();
}

/**
 * Print the help for the temperature subcommands.
 */
static void emit_temperature_help(void)
{
    printf("temperature commands:\n\n");
    printf(TEMPERATURE_HELP_COMMAND "\n");
    printf("  show this help.\n");
    printf("\n");
    printf(TEMPERATURE_READ_COMMAND "\n");
    printf("  read the current temperature and print it\n");
}

/**
 * Read and print the temperature.
 */
static void read_and_print_temperature(void)
{
    float temperature = get_last_temp();
    // last_temp is converted to the configured unit when it is sampled, so the
    // conditional logic is really just for the correct label.
    printf("Temperature is %.1f°", temperature);
    if (current_config.use_celsius) {
        printf("C\n");
    }
    else {
        printf("F\n");
    }
}

/**
 * Temperature tasks - parses commands and executes them appropriately.
 *
 * @param argc [in]  Number of arguments, including the command itself.
 * @param argv [in]  Arguments, including the command itself.
 *
 * @return 0 if success
 * @return 1 if error
 */
static int tasks_temperature(int argc, char **argv)
{
    // assume failure.
    int retval = 1;

    if (argc == 1) {
        // no commands, print help
        emit_temperature_help();
        retval = 0;
    }
    else {
        if (strcmp(argv[1], TEMPERATURE_HELP_COMMAND) == 0) {
            emit_temperature_help();
            retval = 0;
        }
        else if (strcmp(argv[1], TEMPERATURE_READ_COMMAND) == 0) {
            read_and_print_temperature();
            retval = 0;
        }
        else {
            printf("Error: Unknown command: %s\n", argv[1]);
        }
    }

    return retval;
}

static void register_tasks(void)
{
    const esp_console_cmd_t cmd = {
        .command = "temperature",
        .help = "Temperature commands",
        .hint = NULL,
        .func = &tasks_temperature,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
