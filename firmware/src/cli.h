/*
 * Pico Con Command Line Framework
 * WHowe <github.com/whowechina>
 */

#ifndef CLI_H
#define CLI_H


typedef void (*cmd_handler_t)(int argc, char *argv[]);

void cli_init(const char *prompt, const char *logo);
void cli_register(const char *cmd, cmd_handler_t handler, const char *help);
void cli_run();
void cli_fps_count(int core);

int cli_extract_non_neg_int(const char *param, int len);
int cli_match_prefix(const char *str[], int num, const char *prefix);

extern const char *built_time;
#endif
