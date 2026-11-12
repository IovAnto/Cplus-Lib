/**
 * @file cp_args.h
 * @brief Modern, zero-allocation CLI argument parser supporting flags, options, and positionals.
 */

#ifndef CPLUS_ARGS_H
#define CPLUS_ARGS_H

#include "cp_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Argument Parser Structures
 * ------------------------------------------------------------------------- */
#define CP_ARGS_MAX_POSITIONALS 64
#define CP_ARGS_MAX_OPTIONS     64
#define CP_ARGS_MAX_FLAGS       64

typedef struct {
    const char *key;
    const char *value;
} cp_arg_kv_t;

typedef struct {
    int argc;
    char **argv;
    const char *program_name;
    const char *positionals[CP_ARGS_MAX_POSITIONALS];
    usize positional_count;
    cp_arg_kv_t options[CP_ARGS_MAX_OPTIONS];
    usize option_count;
    const char *flags[CP_ARGS_MAX_FLAGS];
    usize flag_count;
} cp_args_t;

/* -------------------------------------------------------------------------
 * Parser Operations
 * ------------------------------------------------------------------------- */
cp_args_t cp_args_parse(int argc, char **argv);
bool cp_args_has_flag(const cp_args_t *args, const char *short_flag, const char *long_flag);
const char *cp_args_get_str(const cp_args_t *args, const char *short_opt, const char *long_opt, const char *default_val);
i64 cp_args_get_int(const cp_args_t *args, const char *short_opt, const char *long_opt, i64 default_val);
f64 cp_args_get_float(const cp_args_t *args, const char *short_opt, const char *long_opt, f64 default_val);
const char *cp_args_get_positional(const cp_args_t *args, usize index);
usize cp_args_positional_count(const cp_args_t *args);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define ArgsParse(argc, argv)            cp_args_parse(argc, argv)
#define ArgsHasFlag(a, sf, lf)           cp_args_has_flag(a, sf, lf)
#define ArgsGetStr(a, sf, lf, def)       cp_args_get_str(a, sf, lf, def)
#define ArgsGetInt(a, sf, lf, def)       cp_args_get_int(a, sf, lf, def)
#define ArgsGetFloat(a, sf, lf, def)     cp_args_get_float(a, sf, lf, def)
#define ArgsGetPositional(a, idx)        cp_args_get_positional(a, idx)
#define ArgsPositionalCount(a)           cp_args_positional_count(a)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_args_t (*const parse)(int argc, char **argv);
    bool (*const hasFlag)(const cp_args_t *args, const char *short_flag, const char *long_flag);
    const char *(*const getStr)(const cp_args_t *args, const char *short_opt, const char *long_opt, const char *default_val);
    i64 (*const getInt)(const cp_args_t *args, const char *short_opt, const char *long_opt, i64 default_val);
    f64 (*const getFloat)(const cp_args_t *args, const char *short_opt, const char *long_opt, f64 default_val);
    const char *(*const getPositional)(const cp_args_t *args, usize index);
    usize (*const positionalCount)(const cp_args_t *args);
} cp_args_namespace_t;

static const cp_args_namespace_t args = {
    .parse = cp_args_parse,
    .hasFlag = cp_args_has_flag,
    .getStr = cp_args_get_str,
    .getInt = cp_args_get_int,
    .getFloat = cp_args_get_float,
    .getPositional = cp_args_get_positional,
    .positionalCount = cp_args_positional_count,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_ARGS_H */
