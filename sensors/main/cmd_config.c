/**
 * @file
 * Various configuration console commands.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "config_storage.h"
#include "console.h"
#include "wifi.h"

static const char *TAG = "cmd_config";

#define CONFIG_HELP_COMMAND "help"
#define CONFIG_SHOW_COMMAND "show"
#define CONFIG_SET_COMMAND "set"
#define CONFIG_SAVE_COMMAND "save"
#define CONFIG_SET_SSID "ssid"
#define CONFIG_SET_PASS "pass"
#define CONFIG_SET_NAME "name"
#define CONFIG_SET_TEMP_UNIT "unit"
#define CONFIG_SET_POLL_INTERVAL "polling"
#define CONFIG_SET_URI "uri"

config_storage_t new_config;

static void register_config(void);

void register_configure(void)
{
    register_config();

    // warm up the shadow config
    memcpy(&new_config, &current_config, sizeof(new_config));
}

/**
 * Check to see if there are unsaved config changes.
 *
 * @return true if there are unsaved changes.
 * @return false if there are not unsaved changes.
 */
static bool is_unsaved_changes(void)
{
    return memcmp(&current_config, &new_config, sizeof(current_config));
}

/**
 * Print the config items.
 *
 * @param config [in] configuration to print.
 */
static void emit_config_items(config_storage_t *config)
{
    printf("\tStation Name: \t%s\n", config->station_name);
    printf("\tSSID:    \t%s\n", config->ssid);
    printf("\tPassword:\t%s\n", config->pass);
    if (config->use_celsius) {
        printf("\tTemp Unit:\tC\n");
    }
    else {
        printf("\tTemp Unit:\tF\n");
    }
    printf("\tPolling:\t%us\n", config->poll_time_sec);
    printf("\tURI:\t\t%s\n", config->uri);
}

/**
 * Print the configuration.
 */
static void emit_config(void)
{
    printf("== Running config ==\n");
    emit_config_items(&current_config);

    if (is_unsaved_changes()) {
        printf("\n");
        printf("== New config ==\n");
        emit_config_items(&new_config);
    }
}

/**
 * Print the set specific help.
 */
static void emit_set_help(void)
{
    printf(CONFIG_SET_COMMAND " <item> <value>\n");
    printf("  set a configuration item to given value.\n");
    printf("\n");
    printf("  possible items:\n");
    printf("    " CONFIG_SET_SSID " = SSID of AP (%d char max).\n",
           MAX_SSID_LEN);
    printf("    " CONFIG_SET_PASS
           " = password for AP (%d char max).\n", MAX_PASSPHRASE_LEN);
    printf("    " CONFIG_SET_NAME
           " = the name of this station (%d char max).\n"
           "        Note: This is used for the both the DHCP client name and\n"
           "        is sent as part of the CoAP payload, so format it "
                    "accordingly.\n",
           MAX_STATION_NAME_LEN);
    printf("        Note: surround the name with quotes if you use spaces.\n");
    printf("    " CONFIG_SET_TEMP_UNIT
           " = the temp unit used by this unit (C or F)\n");
    printf("    " CONFIG_SET_POLL_INTERVAL
           " = the time between temperature checks in seconds\n"
           "            (max %u).\n", UINT16_MAX);
    printf("        Note: this has battery implications. Lower values will\n"
           "        consume more battery, higher values will be less responsive.\n");
    printf("    " CONFIG_SET_URI
           " = URI (%d char max).\n"
           "        Note: This should be of the form:\n"
           "coap://host/url\n", MAX_URI_LEN);
    printf("\n");
}

/**
 * Print the help for the config subcommands.
 */
static void emit_config_help(void)
{
    printf("config commands:\n\n");
    printf(CONFIG_HELP_COMMAND "\n");
    printf("  show this help.\n");
    printf("\n");
    printf(CONFIG_SHOW_COMMAND "\n");
    printf("  show the current configuration.\n");
    printf("\n");
    printf(CONFIG_SAVE_COMMAND "\n");
    printf("  save and apply the current configuration.\n");
    printf("\n");

    emit_set_help();
}

/**
 * Handle our set command.
 *
 * @note No shifting of arguments is done here - the full argument stack is passed in.
 *
 * @param argc [in]  Number of arguments, including the command itself.
 * @param argv [in]  Arguments, including the command itself.
 *
 * @return 0 on success
 * @return 1 on failure
 */
static int handle_set(int argc, char **argv)
{
    // Assume failure.
    int retval = 1;
    int temp;

    // at this point, argument should be like:
    //     config set ssid frobz
    if (argc != 4) {
        printf("Error: Incorrect number of arguments.\n\n");
        printf("config set commands:\n\n");
        emit_set_help();
    }
    else {
        if (strcmp(argv[2], CONFIG_SET_SSID) == 0) {
            if (strlen(argv[3]) > MAX_SSID_LEN) {
                printf("Error: SSID too long, maximum is %d characters.\n", MAX_SSID_LEN);
            }
            else {
                strncpy(new_config.ssid, argv[3], sizeof(new_config.ssid));
                retval = 0;
            }
        }
        else if (strcmp(argv[2], CONFIG_SET_PASS) == 0) {
            if (strlen(argv[3]) > MAX_PASSPHRASE_LEN) {
                printf("Error: password too long, maximum is %d characters.\n",
                       MAX_PASSPHRASE_LEN);
            }
            else {
                strncpy(new_config.pass, argv[3], sizeof(new_config.pass));
                retval = 0;
            }
        }
        else if (strcmp(argv[2], CONFIG_SET_NAME) == 0) {
            if (strlen(argv[3]) > MAX_STATION_NAME_LEN) {
                printf("Error: station name too long, "
                       "maximum is %d characters.\n",
                       MAX_STATION_NAME_LEN);
            }
            else {
                strncpy(new_config.station_name, argv[3],
                        sizeof(new_config.station_name));
                retval = 0;
            }
        }
        else if (strcmp(argv[2], CONFIG_SET_TEMP_UNIT) == 0) {
            if (strlen(argv[3]) != 1) {
                printf("Error: temp unit should be 'C' or 'F'");
            }
            else {
                if (argv[3][0] == 'C') {
                    new_config.use_celsius = true;
                    retval = 0;
                }
                else if (argv[3][0] == 'F') {
                    new_config.use_celsius = false;
                    retval = 0;
                }
                else {
                    printf("Error: temp unit should be 'C' or 'F'");
                }
            }
        }
        else if (strcmp(argv[2], CONFIG_SET_POLL_INTERVAL) == 0) {
            temp = atoi(argv[3]);
            if (temp > UINT16_MAX) {
                printf("Error: polling interval max is %u seconds\n", UINT16_MAX);
            }
            else if (temp < 1) {
                printf("Error: polling interval minimum is 1 second\n");
            }
            else {
                new_config.poll_time_sec = (uint16_t)temp;
                retval = 0;
            }
        }
        else if (strcmp(argv[2], CONFIG_SET_URI) == 0) {
            if (strlen(argv[3]) > MAX_URI_LEN) {
                printf("Error: uri too long, maximum is %d characters.\n",
                       MAX_URI_LEN);
            }
            else {
                strncpy(new_config.uri, argv[3],
                        sizeof(new_config.uri));
                retval = 0;
            }
        }
        else {
            printf("Error: Unknown config set command %s\n", argv[2]);
        }
    }

    return retval;
}

/**
 * Handle our save command.
 *
 * @return 0 on success
 * @return 1 on failure
 */
static bool handle_save()
{
    // Assume failure
    int retval = 1;

    printf("Saving config...\n");

    // Save config
    if (!write_config_to_nvs(&new_config)) {
        printf("Error saving configuration.\n");
    }
    // Reread config
    else if (!read_config_from_nvs(&current_config)) {
        printf("Error reading configuration.\n");
    }
    else {
        printf("... config saved\n");

        // Shadow current config to new config (so they are the same)
        memcpy(&new_config, &current_config, sizeof(new_config));

        set_console_prompt_text();
        wifi_restart();

        // If we got here, everything succeeded and we are therefore happy.
        retval = 0;
    }

    return retval;
}

/**
 * Config tasks - parses commands and executes them appropriately.
 *
 * @param argc [in]  Number of arguments, including the command itself.
 * @param argv [in]  Arguments, including the command itself.
 *
 * @return 0 if success
 * @return 1 if error
 */
static int tasks_config(int argc, char **argv)
{
    // assume failure.
    int retval = 1;

    if (argc == 1) {
        // no commands, print help
        emit_config_help();
        retval = 0;
    }
    else {
        if (strcmp(argv[1], CONFIG_HELP_COMMAND) == 0) {
            emit_config_help();
            retval = 0;
        }
        else if (strcmp(argv[1], CONFIG_SHOW_COMMAND) == 0) {
            emit_config();
            retval = 0;
        }
        else if (strcmp(argv[1], CONFIG_SET_COMMAND) == 0) {
            retval = handle_set(argc, argv);
        }
        else if (strcmp(argv[1], CONFIG_SAVE_COMMAND) == 0) {
            retval = handle_save();
        }
        else {
            printf("Error: Unknown command: %s\n", argv[1]);
        }
    }

    if (is_unsaved_changes()) {
        printf("\nWarning: config has unsaved changes.\n");
    }

    return retval;
}

/**
 * Register the config command.
 */
static void register_config(void)
{
    const esp_console_cmd_t cmd = {
        .command = "config",
        .help = "Configuration operations",
        .hint = NULL,
        .func = &tasks_config,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
