#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "config.h"
#include "touch.h"
#include "save.h"

#define SENSE_LIMIT_MAX 9
#define SENSE_LIMIT_MIN -9

#define MAX_COMMANDS 20
#define MAX_PARAMETERS 5
#define MAX_PARAMETER_LENGTH 20

const char *mai_prompt = "mai_pico>";

typedef void (*cmd_handler_t)(int argc, char *argv[]);

static const char *commands[MAX_COMMANDS];
static cmd_handler_t handlers[MAX_COMMANDS];
static int num_commands = 0;

static void register_command(const char *cmd, cmd_handler_t handler)
{
    if (num_commands < MAX_COMMANDS) {
        commands[num_commands] = cmd;
        handlers[num_commands] = handler;
        num_commands++;
    }
}

// return -1 if not matched, return -2 if ambiguous
static int match_prefix(const char *str[], int num, const char *prefix)
{
    int match = -1;
    bool found = false;

    for (int i = 0; (i < num) && str[i]; i++) {
        if (strncmp(str[i], prefix, strlen(prefix)) == 0) {
            if (found) {
                return -2;
            }
            found = true;
            match = i;
        }
    }

    return match;
}

static void handle_help(int argc, char *argv[])
{
    printf("Available commands:\n");
    for (int i = 0; i < num_commands; i++) {
        printf("%s\n", commands[i]);
    }
}

static void disp_colors()
{
    printf("[Colors]\n");
    printf("  Key on: %06x, off: %06x\n", 
           mai_cfg->colors.key_on, mai_cfg->colors.key_off);
}

static void disp_style()
{
    printf("[Style]\n");
    printf("  Key: %d, Level: %d\n", mai_cfg->style.key, mai_cfg->style.level);
}

static void disp_sense()
{
    printf("[Sense]\n");
    printf("  Filter: %d, %d\n", mai_cfg->sense.filter >> 4, mai_cfg->sense.filter & 0xf);
    printf("  Sensitivity (global: %+d):\n", mai_cfg->sense.global);
    printf("    | 1| 2| 3| 4| 5| 6| 7| 8| 9|10|11|12|13|14|15|16|\n");
    printf("  ---------------------------------------------------\n");
    printf("  A |");
    for (int i = 0; i < 16; i++) {
        printf("%+2d|", mai_cfg->sense.keys[i * 2]);
    }
    printf("\n  B |");
    for (int i = 0; i < 16; i++) {
        printf("%+2d|", mai_cfg->sense.keys[i * 2 + 1]);
    }
    printf("\n");
    printf("  Debounce (touch, release): %d, %d\n",
           mai_cfg->sense.debounce_touch, mai_cfg->sense.debounce_release);
}

static void disp_hid()
{
    printf("[HID]\n");
    printf("  Joy: %s, NKRO: %s.\n", 
           mai_cfg->hid.joy ? "on" : "off",
           mai_cfg->hid.nkro ? "on" : "off" );
}

void handle_display(int argc, char *argv[])
{
    const char *usage = "Usage: display [colors|style|tof|sense|hid]\n";
    if (argc > 1) {
        printf(usage);
        return;
    }

    if (argc == 0) {
        disp_colors();
        disp_style();
        disp_sense();
        disp_hid();
        return;
    }

    const char *choices[] = {"colors", "style", "sense", "hid"};
    switch (match_prefix(choices, 5, argv[0])) {
        case 0:
            disp_colors();
            break;
        case 1:
            disp_style();
            break;
        case 2:
            disp_sense();
            break;
        case 3:
            disp_hid();
            break;
        default:
            printf(usage);
            break;
    }
}

static int fps[2];
void fps_count(int core)
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

static void handle_hid(int argc, char *argv[])
{
    const char *usage = "Usage: hid <joy|nkro|both>\n";
    if (argc != 1) {
        printf(usage);
        return;
    }

    const char *choices[] = {"joy", "nkro", "both"};
    int match = match_prefix(choices, 3, argv[0]);
    if (match < 0) {
        printf(usage);
        return;
    }

    mai_cfg->hid.joy = ((match == 0) || (match == 2)) ? 1 : 0;
    mai_cfg->hid.nkro = ((match == 1) || (match == 2)) ? 1 : 0;
    config_changed();
    disp_hid();
}

static int extract_non_neg_int(const char *param, int len)
{
    if (len == 0) {
        len = strlen(param);
    }
    int result = 0;
    for (int i = 0; i < len; i++) {
        if (!isdigit(param[i])) {
            return -1;
        }
        result = result * 10 + param[i] - '0';
    }
    return result;
}

static void handle_filter(int argc, char *argv[])
{
    const char *usage = "Usage: filter <first> <second>\n"
                        "  first, second: 0..3\n";
    if ((argc < 2) || (argc > 2)) {
        printf(usage);
        return;
    }

    int ffi = extract_non_neg_int(argv[0], 0);
    int sfi = extract_non_neg_int(argv[1], 0);

    if ((ffi < 0) || (ffi > 3) || (sfi < 0) || (sfi > 3)) {
        printf(usage);
        return;
    }

    mai_cfg->sense.filter = (ffi << 4) | sfi;

    touch_update_config();
    config_changed();
    disp_sense();
}

static uint8_t *extract_key(const char *param)
{
    int len = strlen(param);

    int offset;
    if (toupper(param[len - 1]) == 'A') {
        offset = 0;
    } else if (toupper(param[len - 1]) == 'B') {
        offset = 1;
    } else {
        return NULL;
    }

    int id = extract_non_neg_int(param, len - 1) - 1;
    if ((id < 0) || (id > 15)) {
        return NULL;
    }

    return &mai_cfg->sense.keys[id * 2 + offset];
}

static void sense_do_op(int8_t *target, char op)
{
    if (op == '+') {
        if (*target < SENSE_LIMIT_MAX) {
            (*target)++;
        }
    } else if (op == '-') {
        if (*target > SENSE_LIMIT_MIN) {
            (*target)--;
        }
    } else if (op == '0') {
        *target = 0;
    }
}

static void handle_sense(int argc, char *argv[])
{
    const char *usage = "Usage: sense [key|*] <+|-|0>\n"
                        "Example:\n"
                        "  >sense +\n"
                        "  >sense -\n"
                        "  >sense 1A +\n"
                        "  >sense 13B -\n";
                        "  >sense * 0\n";
    if ((argc < 1) || (argc > 2)) {
        printf(usage);
        return;
    }

    const char *op = argv[argc - 1];
    if ((strlen(op) != 1) || !strchr("+-0", op[0])) {
        printf(usage);
        return;
    }

    if (argc == 1) {
        sense_do_op(&mai_cfg->sense.global, op[0]);
    } else {
        if (strcmp(argv[0], "*") == 0) {
            for (int i = 0; i < 32; i++) {
                sense_do_op(&mai_cfg->sense.keys[i], op[0]);
            }
        } else {
            uint8_t *key = extract_key(argv[0]);
            if (!key) {
                printf(usage);
                return;
            }
            sense_do_op(key, op[0]);
        }
    }

    touch_update_config();
    config_changed();
    disp_sense();
}

static void handle_debounce(int argc, char *argv[])
{
    const char *usage = "Usage: debounce <touch> [release]\n"
                        "  touch, release: 0..7\n";
    if ((argc < 1) || (argc > 2)) {
        printf(usage);
        return;
    }

    int touch = mai_cfg->sense.debounce_touch;
    int release = mai_cfg->sense.debounce_release;
    if (argc >= 1) {
        touch = extract_non_neg_int(argv[0], 0);
    }
    if (argc == 2) {
        release = extract_non_neg_int(argv[1], 0);
    }

    if ((touch < 0) || (release < 0) ||
        (touch > 7) || (release > 7)) {
        printf(usage);
        return;
    }

    mai_cfg->sense.debounce_touch = touch;
    mai_cfg->sense.debounce_release = release;

    touch_update_config();
    config_changed();
    disp_sense();
}

static void handle_save()
{
    save_request(true);
}

static void handle_factory_reset()
{
    config_factory_reset();
    printf("Factory reset done.\n");
}

void cmd_init()
{
    register_command("?", handle_help);
    register_command("display", handle_display);
    register_command("fps", handle_fps);
    register_command("hid", handle_hid);
    register_command("filter", handle_filter);
    register_command("sense", handle_sense);
    register_command("debounce", handle_debounce);
    register_command("save", handle_save);
    register_command("factory", config_factory_reset);
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
           (argv[argc] = strtok(NULL, " \n")) != NULL) {
        argc++;
    }

    int match = match_prefix(commands, num_commands, cmd);
    if (match == -2) {
        printf("Ambiguous command.\n");
        return;
    }
    if (match == -1) {
        printf("Unknown command.\n");
        handle_help(0, NULL);
        return;
    }

    handlers[match](argc, argv);
}

void cmd_run()
{
    int c = getchar_timeout_us(0);
    if (c == EOF) {
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

    printf(mai_prompt);
}
