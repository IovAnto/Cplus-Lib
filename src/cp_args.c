/**
 * @file cp_args.c
 * @brief Implementation of modern, zero-allocation CLI argument parser.
 */

#include "cplus/cp_args.h"
#include <string.h>
#include <stdlib.h>

cp_args_t cp_args_parse(int argc, char **argv) {
    cp_args_t res;
    memset(&res, 0, sizeof(res));
    res.argc = argc;
    res.argv = argv;
    if (argc > 0 && argv) {
        res.program_name = argv[0];
    }

    bool parse_opts = true;
    int i = 1;

    while (i < argc) {
        const char *arg = argv[i];
        if (!arg) {
            i++;
            continue;
        }

        if (parse_opts && strcmp(arg, "--") == 0) {
            parse_opts = false;
            i++;
            continue;
        }

        if (parse_opts && arg[0] == '-' && arg[1] != '\0') {
            /* Record as flag */
            if (res.flag_count < CP_ARGS_MAX_FLAGS) {
                res.flags[res.flag_count++] = arg;
            }

            const char *eq = strchr(arg, '=');
            if (eq) {
                /* Key-Value with '=': e.g. --port=8080 */
                if (res.option_count < CP_ARGS_MAX_OPTIONS) {
                    res.options[res.option_count].key = arg;
                    res.options[res.option_count].value = eq + 1;
                    res.option_count++;
                }
            } else if (i + 1 < argc && argv[i + 1] && argv[i + 1][0] != '-') {
                /* Key followed by value: e.g. --port 8080 */
                if (res.option_count < CP_ARGS_MAX_OPTIONS) {
                    res.options[res.option_count].key = arg;
                    res.options[res.option_count].value = argv[i + 1];
                    res.option_count++;
                }
                /* Also preserve argv[i+1] in positionals in case arg was a boolean flag */
                if (res.positional_count < CP_ARGS_MAX_POSITIONALS) {
                    res.positionals[res.positional_count++] = argv[i + 1];
                }
                i++; /* consumed value */
            }
        } else {
            /* Positional argument */
            if (res.positional_count < CP_ARGS_MAX_POSITIONALS) {
                res.positionals[res.positional_count++] = arg;
            }
        }
        i++;
    }
    return res;
}

bool cp_args_has_flag(const cp_args_t *args, const char *short_flag, const char *long_flag) {
    if (!args) return false;
    for (usize i = 0; i < args->flag_count; ++i) {
        const char *f = args->flags[i];
        const char *eq = strchr(f, '=');
        usize klen = eq ? (usize)(eq - f) : strlen(f);

        if (short_flag && strncmp(f, short_flag, klen) == 0 && strlen(short_flag) == klen) {
            return true;
        }
        if (long_flag && strncmp(f, long_flag, klen) == 0 && strlen(long_flag) == klen) {
            return true;
        }
    }
    return false;
}

const char *cp_args_get_str(const cp_args_t *args, const char *short_opt, const char *long_opt, const char *default_val) {
    if (!args) return default_val;
    for (usize i = 0; i < args->option_count; ++i) {
        const char *k = args->options[i].key;
        const char *eq = strchr(k, '=');
        usize klen = eq ? (usize)(eq - k) : strlen(k);

        if (short_opt && strncmp(k, short_opt, klen) == 0 && strlen(short_opt) == klen) {
            return args->options[i].value;
        }
        if (long_opt && strncmp(k, long_opt, klen) == 0 && strlen(long_opt) == klen) {
            return args->options[i].value;
        }
    }
    return default_val;
}

i64 cp_args_get_int(const cp_args_t *args, const char *short_opt, const char *long_opt, i64 default_val) {
    const char *s = cp_args_get_str(args, short_opt, long_opt, NULL);
    if (!s) return default_val;
    return (i64)strtoll(s, NULL, 0);
}

f64 cp_args_get_float(const cp_args_t *args, const char *short_opt, const char *long_opt, f64 default_val) {
    const char *s = cp_args_get_str(args, short_opt, long_opt, NULL);
    if (!s) return default_val;
    return strtod(s, NULL);
}

const char *cp_args_get_positional(const cp_args_t *args, usize index) {
    if (!args || index >= args->positional_count) return NULL;
    return args->positionals[index];
}

usize cp_args_positional_count(const cp_args_t *args) {
    return args ? args->positional_count : 0;
}
