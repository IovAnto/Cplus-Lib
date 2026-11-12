/**
 * @file cp_json.h
 * @brief Zero-allocation micro JSON parser and query engine operating over StrView slices.
 */

#ifndef CPLUS_JSON_H
#define CPLUS_JSON_H

#include "cp_core.h"
#include "cp_str.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * JSON Value Types & Structure
 * ------------------------------------------------------------------------- */
typedef enum {
    CP_JSON_INVALID = 0,
    CP_JSON_NULL,
    CP_JSON_BOOL,
    CP_JSON_NUMBER,
    CP_JSON_STRING,
    CP_JSON_ARRAY,
    CP_JSON_OBJECT
} cp_json_type_t;

typedef struct {
    StrView raw;
    cp_json_type_t type;
} cp_json_val_t;

/* -------------------------------------------------------------------------
 * Core JSON Operations
 * ------------------------------------------------------------------------- */
cp_json_val_t cp_json_parse(const char *json_str, usize len);
cp_json_val_t cp_json_get(cp_json_val_t obj, const char *key);
cp_json_val_t cp_json_at(cp_json_val_t arr, usize index);
usize cp_json_len(cp_json_val_t val);

StrView cp_json_as_view(cp_json_val_t val);
i64 cp_json_as_int(cp_json_val_t val, i64 default_val);
f64 cp_json_as_float(cp_json_val_t val, f64 default_val);
bool cp_json_as_bool(cp_json_val_t val, bool default_val);
bool cp_json_is_null(cp_json_val_t val);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define JsonParse(str, len)              cp_json_parse(str, len)
#define JsonGet(obj, key)                cp_json_get(obj, key)
#define JsonAt(arr, index)               cp_json_at(arr, index)
#define JsonLen(val)                     cp_json_len(val)
#define JsonAsView(val)                  cp_json_as_view(val)
#define JsonAsInt(val, def)              cp_json_as_int(val, def)
#define JsonAsFloat(val, def)            cp_json_as_float(val, def)
#define JsonAsBool(val, def)             cp_json_as_bool(val, def)
#define JsonIsNull(val)                  cp_json_is_null(val)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_json_val_t (*const parse)(const char *json_str, usize len);
    cp_json_val_t (*const get)(cp_json_val_t obj, const char *key);
    cp_json_val_t (*const at)(cp_json_val_t arr, usize index);
    usize (*const len)(cp_json_val_t val);
    StrView (*const asView)(cp_json_val_t val);
    i64 (*const asInt)(cp_json_val_t val, i64 default_val);
    f64 (*const asFloat)(cp_json_val_t val, f64 default_val);
    bool (*const asBool)(cp_json_val_t val, bool default_val);
    bool (*const isNull)(cp_json_val_t val);
} cp_json_namespace_t;

static const cp_json_namespace_t json_api = {
    .parse = cp_json_parse,
    .get = cp_json_get,
    .at = cp_json_at,
    .len = cp_json_len,
    .asView = cp_json_as_view,
    .asInt = cp_json_as_int,
    .asFloat = cp_json_as_float,
    .asBool = cp_json_as_bool,
    .isNull = cp_json_is_null,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_JSON_H */
