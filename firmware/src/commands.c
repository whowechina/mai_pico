#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "tusb.h"

#include "touch.h"
#include "config.h"
#include "save.h"
#include "cli.h"

#define SENSE_LIMIT_MAX 9
#define SENSE_LIMIT_MIN -9

static void disp_color()
{
    printf("[Color]\n");
    printf("  Key on: %06x, off: %06x\n  Level: %d\n",
           mai_cfg->color.key_on, mai_cfg->color.key_off, mai_cfg->color.level);
}

static void disp_sense()
{
    printf("[Sense]\n");
    printf("  Filter: %u, %u, %u\n", mai_cfg->sense.filter >> 6,
                                    (mai_cfg->sense.filter >> 4) & 0x03,
                                    mai_cfg->sense.filter & 0x07);
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
    const char *nkro[] = {"off", "key1", "key2"};
    printf("  Joy: %s, NKRO: %s\n", mai_cfg->hid.joy ? "on" : "off",
           mai_cfg->hid.nkro <= 2 ? nkro[mai_cfg->hid.nkro] : "key1");
}

void handle_display(int argc, char *argv[])
{
    const char *usage = "Usage: display [color|sense|hid]\n";
    if (argc > 1) {
        printf(usage);
        return;
    }

    if (argc == 0) {
        disp_color();
        disp_sense();
        disp_hid();
        return;
    }

    const char *choices[] = {"color", "sense", "hid"};
    switch (cli_match_prefix(choices, 3, argv[0])) {
        case 0:
            disp_color();
            break;
        case 1:
            disp_sense();
            break;
        case 2:
            disp_hid();
            break;
        default:
            printf(usage);
            break;
    }
}

static void handle_level(int argc, char *argv[])
{
    const char *usage = "Usage: level <0..255>\n";
    if (argc != 1) {
        printf(usage);
        return;
    }

    int level = cli_extract_non_neg_int(argv[0], 0);
    if ((level < 0) || (level > 255)) {
        printf(usage);
        return;
    }

    mai_cfg->color.level = level;
    config_changed();
    disp_color();
}

static void handle_stat(int argc, char *argv[])
{
    if (argc == 0) {
        for (int col = 0; col < 4; col++) {
            printf(" %2dA |", col * 4 + 1);
            for (int i = 0; i < 4; i++) {
                printf("%6u|", touch_count(col * 8 + i * 2));
            }
            printf("\n   B |");
            for (int i = 0; i < 4; i++) {
                printf("%6u|", touch_count(col * 8 + i * 2 + 1));
            }
            printf("\n");
        }
    } else if ((argc == 1) &&
               (strncasecmp(argv[0], "reset", strlen(argv[0])) == 0)) {
        touch_reset_stat();
    } else {
        printf("Usage: stat [reset]\n");
    }
}

static void handle_hid(int argc, char *argv[])
{
    const char *usage = "Usage: hid <joy|key1|key2\n";
    if (argc != 1) {
        printf(usage);
        return;
    }

    const char *choices[] = {"joy", "key1", "key2"};
    int match = cli_match_prefix(choices, 3, argv[0]);
    if (match < 0) {
        printf(usage);
        return;
    }

    switch (match) {
            break;
        case 1:
            mai_cfg->hid.joy = 0;
            mai_cfg->hid.nkro = 1;
            break;
        case 2:
            mai_cfg->hid.joy = 0;
            mai_cfg->hid.nkro = 2;
            break;
        case 0:
        default:
            mai_cfg->hid.joy = 1;
            mai_cfg->hid.nkro = 0;
            break;
    }
    config_changed();
    disp_hid();
}

static void handle_filter(int argc, char *argv[])
{
    const char *usage = "Usage: filter <first> <second> [interval]\n"
                        "    first: First iteration [0..3]\n"
                        "   second: Second iteration [0..3]\n"
                        " interval: Interval of second iterations [0..7]\n";
    if ((argc < 2) || (argc > 3)) {
        printf(usage);
        return;
    }

    int ffi = cli_extract_non_neg_int(argv[0], 0);
    int sfi = cli_extract_non_neg_int(argv[1], 0);
    int intv = mai_cfg->sense.filter & 0x07;
    if (argc == 3) {
        intv = cli_extract_non_neg_int(argv[2], 0);
    }

    if ((ffi < 0) || (ffi > 3) || (sfi < 0) || (sfi > 3) ||
        (intv < 0) || (intv > 7)) {
        printf(usage);
        return;
    }

    mai_cfg->sense.filter = (ffi << 6) | (sfi << 4) | intv;

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

    int id = cli_extract_non_neg_int(param, len - 1) - 1;
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
        touch = cli_extract_non_neg_int(argv[0], 0);
    }
    if (argc == 2) {
        release = cli_extract_non_neg_int(argv[1], 0);
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

static void handle_raw()
{
    printf("Key raw readings:\n");
    const uint16_t *raw = touch_raw();
    printf("|");
    for (int i = 0; i < 16; i++) {
        printf("%3d|", raw[i * 2]);
    }
    printf("\n|");
    for (int i = 0; i < 16; i++) {
        printf("%3d|", raw[i * 2 + 1]);
    }
    printf("\n");
}

static void handle_whoami()
{
    const char *msg[] = {"\nThis is Command Line port.\n", "\nThis is Touch port.\n", "\nThis is LED port.\n"};
    for (int i = 0; i < 3; i++) {
        tud_cdc_n_write(i, msg[i], strlen(msg[i]));
        tud_cdc_n_write_flush(i);
    }
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

void commands_init()
{
    cli_register("display", handle_display, "Display all config.");
    cli_register("level", handle_level, "Set LED brightness level.");
    cli_register("stat", handle_stat, "Display or reset statistics.");
    cli_register("hid", handle_hid, "Set HID mode.");
    cli_register("filter", handle_filter, "Set pre-filter config.");
    cli_register("sense", handle_sense, "Set sensitivity config.");
    cli_register("debounce", handle_debounce, "Set debounce config.");
    cli_register("raw", handle_raw, "Show key raw readings.");
    cli_register("whoami", handle_whoami, "Identify each com port.");
    cli_register("save", handle_save, "Save config to flash.");
    cli_register("factory", config_factory_reset, "Reset everything to default.");
}
