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
#define CONFIG_SET_CACHE_AP "cache_ap"
#define CONFIG_SET_USE_DHCP "use_dhcp"
#define CONFIG_SET_IP_ADDR "ip_addr"
#define CONFIG_SET_NETMASK "netmask"
#define CONFIG_SET_GATEWAY "gateway"
#define CONFIG_SET_DNS "dns"
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
    // Max IPv4 addr is 15 chars + NUL
    char temp_buf[MAX_IPV4_LEN];
    char * ret_buf;

    printf("\tStation Name: \t%s\n", config->station_name);
    printf("\tSSID:    \t%s\n", config->ssid);
    printf("\tPassword:\t%s\n", config->pass);
    printf("\tCache AP Info:\t");
    if (config->cache_ap_info) {
        printf("Yes");
    }
    else {
        printf("No");
    }
    printf("\n");
    printf("\tUse DHCP:\t");
    if (config->use_dhcp) {
        printf("Yes");
    }
    else {
        printf("No");
    }
    printf("\n");
    // if DHCP is off, print static
    if (!config->use_dhcp) {
        // Note on ip4addr_ntoa_r:
        // temp_buf is supplied by us and ret_buf either a pointer back to it or
        // NULL if the buffer was too small

        ret_buf = ip4addr_ntoa_r(&config->ipaddr, temp_buf, sizeof(temp_buf));
        if (ret_buf == NULL) {
            ESP_LOGE(TAG, "Buffer too small when formatting IP address for printing");
        }
        else {
            printf("\tIP Address:\t%s\n", ret_buf);
        }

        ret_buf = ip4addr_ntoa_r(&config->netmask, temp_buf, sizeof(temp_buf));
        if (ret_buf == NULL) {
            ESP_LOGE(TAG, "Buffer too small when formatting netmask for printing");
        }
        else {
            printf("\tNetmask:\t%s\n", ret_buf);
        }

        // gateway is optional, only decode if nonzero
        if (config->gateway.addr == 0) {
                printf("\tGateway:\tNot set\n");            
        } 
        else {
            ret_buf = ip4addr_ntoa_r(&config->gateway, temp_buf, sizeof(temp_buf));
            if (ret_buf == NULL) {
                ESP_LOGE(TAG, "Buffer too small when formatting gateway for printing");
            }
            else {
                printf("\tGateway:\t%s\n", ret_buf);
            }            
        }

        // same for DNS
        if (config->dns.addr == 0) {
                printf("\tDNS Server:\tNot set\n");            
        } 
        else {
            ret_buf = ip4addr_ntoa_r(&config->dns, temp_buf, sizeof(temp_buf));
            if (ret_buf == NULL) {
                ESP_LOGE(TAG, "Buffer too small when formatting DNS for printing");
            }
            else {
                printf("\tDNS Server:\t%s\n", ret_buf);
            }            
        }
    }

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
    printf("    " CONFIG_SET_CACHE_AP
           " = whether or not to cache the current AP info during sleep (Y or N).\n"
           "        Enabling this saves power because it doesn't rescan for\n"
           "        the best AP after sleeping, which assumes that the\n"
           "        network won't change while it is sleeping.\n"
           "        Disabling this will respond better to changes in network\n"
           "        topology, but will reduce battery life.\n"
           "        In testing, this shortens connection times by about 300ms.\n");
    printf("    " CONFIG_SET_USE_DHCP
           " = whether or not to use a DHCP client to acquire an IP address (Y or N).\n"
           "        Disabling this saves power because it doesn't need to reacquire\n"
           "        an IP address on boot. However, you will need to enter a static\n"
           "        IP configuration.\n"
           "        In testing, this shortens connections times by about 1s.\n");
    printf("    " CONFIG_SET_IP_ADDR
           " = the static IP address to use if USE_DHCP is set to N.\n"
           "        This is ignored if USE_DHCP is set to Y.\n");
    printf("    " CONFIG_SET_NETMASK
           " = the netmask to use if USE_DHCP is set to N.\n"
           "        This is ignored if USE_DHCP is set to Y.\n");
    printf("    " CONFIG_SET_GATEWAY
           " = the gateway to use if USE_DHCP is set to N.\n"
           "        This is ignored if USE_DHCP is set to Y.\n"
           "        This is optional. If unset, the sensor will not be able\n"
           "        to get to the internet (which may be desirable)\n");
    printf("    " CONFIG_SET_DNS
           " = the DNS server to use if USE_DHCP is set to N.\n"
           "        This is ignored if USE_DHCP is set to Y.\n"
           "        This is optional. If unset, the sensor will not be able\n"
           "        to get to perform DNS lookups (which may be desirable)\n");
    printf("    " CONFIG_SET_NAME
           " = the name of this station (%d char max).\n"
           "        Note: This is used for the both the DHCP client name and\n"
           "        is sent as part of the CoAP payload, so format it\n"
           "        accordingly. An invalid format will result in a failure\n"
           "        to set the hostname.",
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
    ip4_addr_t temp_ip;

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
                printf("Error: temp unit should be 'C' or 'F'.\n");
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
                    printf("Error: temp unit should be 'C' or 'F'.\n");
                }
            }
        }
        else if (strcmp(argv[2], CONFIG_SET_CACHE_AP) == 0) {
            if (strlen(argv[3]) != 1) {
                printf("Error: cache AP setting should be 'Y' or 'N'.\n");
            }
            else {
                if (argv[3][0] == 'Y') {
                    new_config.cache_ap_info = true;
                    retval = 0;
                }
                else if (argv[3][0] == 'N') {
                    new_config.cache_ap_info = false;
                    retval = 0;
                }
                else {
                    printf("Error: cache AP setting should be 'Y' or 'N'.\n");
                }
            }
        }        
        else if (strcmp(argv[2], CONFIG_SET_POLL_INTERVAL) == 0) {
            temp = atoi(argv[3]);
            if (temp > UINT16_MAX) {
                printf("Error: polling interval max is %u seconds.\n", UINT16_MAX);
            }
            else if (temp < 1) {
                printf("Error: polling interval minimum is 1 second.\n");
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
        else if (strcmp(argv[2], CONFIG_SET_USE_DHCP) == 0) {
            if (strlen(argv[3]) != 1) {
                printf("Error: use DHCP setting should be 'Y' or 'N'.\n");
            }
            else {
                if (argv[3][0] == 'Y') {
                    new_config.use_dhcp = true;
                    retval = 0;
                }
                else if (argv[3][0] == 'N') {
                    new_config.use_dhcp = false;
                    retval = 0;
                }
                else {
                    printf("Error: use DHCP setting should be 'Y' or 'N'.\n");
                }
            }
        }
        else if (strcmp(argv[2], CONFIG_SET_IP_ADDR) == 0) {
            if (strlen(argv[3]) > MAX_IPV4_LEN) {
                printf("Error: IP address too long, maximum is %d"
                       " characters.\n", MAX_IPV4_LEN);
            }
            else {
                // 1 if IP could be converted to addr, 0 on failure 
                if (ip4addr_aton(argv[3], &temp_ip) == 0) {
                    printf("Error: IP address is invalid. It must be in the"
                           " form 192.168.1.100. (Only IPv4 is supported at"
                           " this time.\n");
                } else {
                    new_config.ipaddr.addr = temp_ip.addr;
                    retval = 0;
                }
            }
        }
        else if (strcmp(argv[2], CONFIG_SET_NETMASK) == 0) {
            if (strlen(argv[3]) > MAX_IPV4_LEN) {
                printf("Error: Netmask IP address too long, maximum is %d"
                       " characters.\n", MAX_IPV4_LEN);
            }
            else {
                // 1 if IP could be converted to addr, 0 on failure 
                if (ip4addr_aton(argv[3], &temp_ip) == 0) {
                    printf("Error: Netmask IP address is invalid. It must be"
                           " in the form 192.168.1.100. (Only IPv4 is"
                           " supported at this time.\n");
                } else {
                    new_config.netmask.addr = temp_ip.addr;
                    retval = 0;
                }
            }
        }
        else if (strcmp(argv[2], CONFIG_SET_GATEWAY) == 0) {
            if (strlen(argv[3]) > MAX_IPV4_LEN) {
                printf("Error: Gateway IP address too long, maximum is %d"
                       " characters.\n", MAX_IPV4_LEN);
            }
            else {
                // 1 if IP could be converted to addr, 0 on failure 
                if (ip4addr_aton(argv[3], &temp_ip) == 0) {
                    printf("Error: Gateway IP address is invalid. It must be"
                           " in the form 192.168.1.100. (Only IPv4 is"
                           " supported at this time.\n");
                } else {
                    new_config.gateway.addr = temp_ip.addr;
                    retval = 0;
                }
            }
        }
        else if (strcmp(argv[2], CONFIG_SET_DNS) == 0) {
            if (strlen(argv[3]) > MAX_IPV4_LEN) {
                printf("Error: DNS server IP address too long, maximum is %d"
                       " characters.\n", MAX_IPV4_LEN);
            }
            else {
                // 1 if IP could be converted to addr, 0 on failure 
                if (ip4addr_aton(argv[3], &temp_ip) == 0) {
                    printf("Error: DNS server IP address is invalid. It must be"
                           " in the form 192.168.1.100. (Only IPv4 is"
                           " supported at this time.\n");
                } else {
                    new_config.dns.addr = temp_ip.addr;
                    retval = 0;
                }
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
