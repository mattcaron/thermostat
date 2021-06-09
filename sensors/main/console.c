/**
 * @file
 *
 * Console parser implementation.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"

#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

#include "console.h"
#include "cmd_system.h"
#include "cmd_config.h"
#include "config_storage.h"
#include "cmd_wifi.h"
#include "cmd_temperature.h"
#include "priorities.h"

// Plain prompt is the station name with a >, a space, and a NUL.
char plain_prompt[MAX_STATION_NAME_LEN+3];
// Prompt is the station name with some extra control characters.
char color_prompt[MAX_STATION_NAME_LEN+14];

void set_console_prompt_text(void)
{
    if (strlen(current_config.station_name) > 0) {
        snprintf(color_prompt, sizeof(color_prompt),
                 LOG_COLOR_I "%s> " LOG_RESET_COLOR,
                 current_config.station_name);

        snprintf(plain_prompt, sizeof(plain_prompt), "%s> ",
                 current_config.station_name);
    }
    else {
        snprintf(color_prompt, sizeof(color_prompt),
                 LOG_COLOR_I "sensor> " LOG_RESET_COLOR);

        snprintf(plain_prompt, sizeof(plain_prompt), "sensor> ");
    }
}

/**
 * Initialize nonvolatile storage.
 */
static void initialize_console(void)
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Configure UART.
     */
    uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
    };

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM,
                                         256, 0, 0, NULL, 0) );
    ESP_ERROR_CHECK( uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
        .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback *) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

#if CONFIG_STORE_HISTORY
    /* Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
#endif
}

/**
 * The console task.
 *
 * Handles console input and does things.
 *
 * @param pvParameters Parameters for the function (unused)
 *
 */
static void console_task(void *pvParameters)
{
    initialize_console();

    bool use_color_prompt = true;
    const char *prompt;

    /* Register commands */
    esp_console_register_help_command();
    register_system();
    register_configure();
    register_wifi();
    register_temperature();

    printf("\n"
           LOG_COLOR(LOG_COLOR_BLUE)
           "Welcome to the ESP sensor module.\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n");

    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        use_color_prompt = false;
#endif //CONFIG_LOG_COLORS
    }

    set_console_prompt_text();

    /* Main loop */
    while(true) {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        if (use_color_prompt) {
            prompt = color_prompt;
        }
        else {
            prompt = plain_prompt;
        }
        char *line = linenoise(prompt);
        if (line == NULL) { /* Ignore empty lines */
            continue;
        }
        /* Add the command to the history */
        linenoiseHistoryAdd(line);
#if CONFIG_STORE_HISTORY
        /* Save command history to filesystem */
        linenoiseHistorySave(HISTORY_PATH);
#endif

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        }
        else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        }
        else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret,
                   esp_err_to_name(ret));
        }
        else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }
}

void start_console(void)
{
    xTaskCreate(console_task, "console", 2048, NULL, CONSOLE_TASK_PRIORITY, NULL);
}
