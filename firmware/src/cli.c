#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "cli.h"
#include "save.h"

#define MAX_COMMANDS 32
#define MAX_PARAMETERS 10
#define MAX_PARAMETER_LENGTH 20

const char *cli_prompt = "cli>";
const char *cli_logo = "CLI";

static const char *commands[MAX_COMMANDS];
static const char *helps[MAX_COMMANDS];
static cmd_handler_t handlers[MAX_COMMANDS];
static int max_cmd_len = 0;

static int num_commands = 0;

void cli_register(const char *cmd, cmd_handler_t handler, const char *help)
{
    if (num_commands < MAX_COMMANDS) {
        commands[num_commands] = cmd;
        handlers[num_commands] = handler;
        helps[num_commands] = help;
        num_commands++;
        if (strlen(cmd) > max_cmd_len) {
            max_cmd_len = strlen(cmd);
        }
    }
}

// return -1 if not matched, return -2 if ambiguous
int cli_match_prefix(const char *str[], int num, const char *prefix)
{
    int match = -1;
    bool found = false;

    for (int i = 0; (i < num) && str[i]; i++) {
        if (strncasecmp(str[i], prefix, strlen(prefix)) == 0) {
            if (found) {
                return -2;
            }
            found = true;
            match = i;
        }
    }

    return match;
}

const char *built_time = __DATE__ " " __TIME__;
static void handle_help(int argc, char *argv[])
{
    printf("%s", cli_logo);
    printf("\tSN: %016llx\n", board_id_64());
    printf("\tBuilt: %s\n\n", built_time);
    printf("Available commands:\n");
    for (int i = 0; i < num_commands; i++) {
        printf("%*s: %s\n", max_cmd_len + 2, commands[i], helps[i]);
    }
}

static int fps[2];
void cli_fps_count(int core)
{
    static uint32_t last[2] = {0};
    static int counter[2] = {0};

    counter[core]++;

    uint32_t now = time_us_32();
    if (now - last[core] < 1000000) {
        return;
    }
    last[core] = now;
    fps[core] = counter[core];
    counter[core] = 0;
}

static void handle_fps(int argc, char *argv[])
{
    printf("FPS: core 0: %d, core 1: %d\n", fps[0], fps[1]);
}

static void handle_update(int argc, char *argv[])
{
    printf("Boot into update mode.\n");
    fflush(stdout);
    sleep_ms(100);
    reset_usb_boot(0, 2);
}
int cli_extract_non_neg_int(const char *param, int len)
{
    if (len == 0) {
        len = strlen(param);
    }
    int result = 0;
    for (int i = 0; i < len; i++) {
        if (!isdigit((uint8_t)param[i])) {
            return -1;
        }
        result = result * 10 + param[i] - '0';
    }
    return result;
}

static char cmd_buf[256];
static int cmd_len = 0;

static void process_cmd()
{
    char *argv[MAX_PARAMETERS];
    int argc;

    char *cmd = strtok(cmd_buf, " \n");

    if (strlen(cmd) == 0) {
        return;
    }

    argc = 0;
    while ((argc < MAX_PARAMETERS) &&
           (argv[argc] = strtok(NULL, " ,\n")) != NULL) {
        argc++;
    }

    int match = cli_match_prefix(commands, num_commands, cmd);
    if (match == -2) {
        printf("Ambiguous command.\n");
        return;
    } else if (match == -1) {
        printf("Unknown command.\n");
        handle_help(0, NULL);
        return;
    }

    handlers[match](argc, argv);
}

void cli_run()
{
    static bool was_connected = false;
    static uint64_t connect_time = 0;
    static bool welcomed = false;
    bool connected = stdio_usb_connected();
    bool just_connected = connected && !was_connected;
    was_connected = connected;
    if (!connected) {
        return;
    }
    if (just_connected) {
        connect_time = time_us_64();
        welcomed = false;
        return;
    }
    if (!welcomed && (time_us_64() - connect_time > 200000)) {
        welcomed = true;
        cmd_len = 0;
        handle_help(0, NULL);
        printf("\n%s", cli_prompt);
    }
    int c = getchar_timeout_us(0);
    if (c < 0) {
        return;
    }

    if (c == '\b' || c == 127) { // both backspace and delete
        if (cmd_len > 0) {
            cmd_len--;
            printf("\b \b");
        }
        return;
    }

    if ((c != '\n') && (c != '\r')) {

        if (cmd_len < sizeof(cmd_buf) - 2) {
            cmd_buf[cmd_len] = c;
            printf("%c", c);
            cmd_len++;
        }
        return;
    }

    cmd_buf[cmd_len] = '\0';
    cmd_len = 0;

    printf("\n");

    process_cmd();

    printf(cli_prompt);
}

void cli_init(const char *prompt, const char *logo)
{
    if (prompt) {
        cli_prompt = prompt;
    }
    if (logo) {
        cli_logo = logo;
    }

    cli_register("?", handle_help, "Display this help message.");
    cli_register("fps", handle_fps, "Display FPS.");
    cli_register("update", handle_update, "Update firmware.");
}
