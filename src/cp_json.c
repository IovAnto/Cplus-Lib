/**
 * @file cp_json.c
 * @brief Zero-allocation micro JSON parser implementation using StrView.
 */

#include "cplus/cp_json.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

static const char *skip_ws(const char *p, const char *end) {
    while (p < end && isspace((unsigned char)*p)) p++;
    return p;
}

static const char *skip_string(const char *p, const char *end) {
    if (p >= end || *p != '"') return p;
    p++;
    while (p < end) {
        if (*p == '\\') {
            p += 2;
        } else if (*p == '"') {
            return p + 1;
        } else {
            p++;
        }
    }
    return end;
}

static const char *skip_val(const char *p, const char *end) {
    p = skip_ws(p, end);
    if (p >= end) return end;

    if (*p == '"') {
        return skip_string(p, end);
    } else if (*p == '{') {
        p++;
        int depth = 1;
        while (p < end && depth > 0) {
            p = skip_ws(p, end);
            if (p >= end) break;
            if (*p == '"') {
                p = skip_string(p, end);
            } else {
                if (*p == '{') depth++;
                else if (*p == '}') depth--;
                p++;
            }
        }
        return p;
    } else if (*p == '[') {
        p++;
        int depth = 1;
        while (p < end && depth > 0) {
            p = skip_ws(p, end);
            if (p >= end) break;
            if (*p == '"') {
                p = skip_string(p, end);
            } else {
                if (*p == '[') depth++;
                else if (*p == ']') depth--;
                p++;
            }
        }
        return p;
    } else {
        /* Number, bool, or null: scan until delimiter */
        while (p < end && *p != ',' && *p != '}' && *p != ']' && !isspace((unsigned char)*p)) {
            p++;
        }
        return p;
    }
}

static cp_json_type_t detect_type(const char *p, const char *end) {
    p = skip_ws(p, end);
    if (p >= end) return CP_JSON_INVALID;
    if (*p == '{') return CP_JSON_OBJECT;
    if (*p == '[') return CP_JSON_ARRAY;
    if (*p == '"') return CP_JSON_STRING;
    if (*p == 't' || *p == 'f') return CP_JSON_BOOL;
    if (*p == 'n') return CP_JSON_NULL;
    if (*p == '-' || isdigit((unsigned char)*p)) return CP_JSON_NUMBER;
    return CP_JSON_INVALID;
}

cp_json_val_t cp_json_parse(const char *json_str, usize len) {
    cp_json_val_t v = { .raw = { NULL, 0 }, .type = CP_JSON_INVALID };
    if (!json_str) return v;
    if (len == 0) len = strlen(json_str);

    const char *start = skip_ws(json_str, json_str + len);
    const char *end = json_str + len;
    while (end > start && isspace((unsigned char)*(end - 1))) end--;

    if (start >= end) return v;

    v.raw = cp_sv_from_parts(start, (usize)(end - start));
    v.type = detect_type(start, end);
    return v;
}

cp_json_val_t cp_json_get(cp_json_val_t obj, const char *key) {
    cp_json_val_t res = { .raw = { NULL, 0 }, .type = CP_JSON_INVALID };
    if (obj.type != CP_JSON_OBJECT || !key || obj.raw.len < 2) return res;

    const char *p = obj.raw.data;
    const char *end = p + obj.raw.len;

    p = skip_ws(p, end);
    if (p >= end || *p != '{') return res;
    p++; /* skip '{' */

    usize key_len = strlen(key);

    while (p < end) {
        p = skip_ws(p, end);
        if (p >= end || *p == '}') break;

        if (*p != '"') return res;
        const char *k_start = p + 1;
        const char *k_end = skip_string(p, end) - 1;
        p = k_end + 1;

        p = skip_ws(p, end);
        if (p >= end || *p != ':') return res;
        p++; /* skip ':' */

        p = skip_ws(p, end);
        const char *v_start = p;
        const char *v_end = skip_val(p, end);

        usize current_klen = (usize)(k_end - k_start);
        if (current_klen == key_len && memcmp(k_start, key, key_len) == 0) {
            res.raw = cp_sv_from_parts(v_start, (usize)(v_end - v_start));
            res.type = detect_type(v_start, v_end);
            return res;
        }

        p = skip_ws(v_end, end);
        if (p < end && *p == ',') {
            p++;
        }
    }
    return res;
}

cp_json_val_t cp_json_at(cp_json_val_t arr, usize index) {
    cp_json_val_t res = { .raw = { NULL, 0 }, .type = CP_JSON_INVALID };
    if (arr.type != CP_JSON_ARRAY || arr.raw.len < 2) return res;

    const char *p = arr.raw.data;
    const char *end = p + arr.raw.len;

    p = skip_ws(p, end);
    if (p >= end || *p != '[') return res;
    p++; /* skip '[' */

    usize current_idx = 0;

    while (p < end) {
        p = skip_ws(p, end);
        if (p >= end || *p == ']') break;

        const char *v_start = p;
        const char *v_end = skip_val(p, end);

        if (current_idx == index) {
            res.raw = cp_sv_from_parts(v_start, (usize)(v_end - v_start));
            res.type = detect_type(v_start, v_end);
            return res;
        }

        current_idx++;
        p = skip_ws(v_end, end);
        if (p < end && *p == ',') {
            p++;
        }
    }
    return res;
}

usize cp_json_len(cp_json_val_t val) {
    if (val.type != CP_JSON_OBJECT && val.type != CP_JSON_ARRAY) return 0;
    if (val.raw.len < 2) return 0;

    const char *p = val.raw.data;
    const char *end = p + val.raw.len;
    p = skip_ws(p, end) + 1; /* skip { or [ */

    char close_char = (val.type == CP_JSON_OBJECT) ? '}' : ']';
    usize count = 0;

    while (p < end) {
        p = skip_ws(p, end);
        if (p >= end || *p == close_char) break;

        if (val.type == CP_JSON_OBJECT) {
            p = skip_string(p, end);
            p = skip_ws(p, end);
            if (p < end && *p == ':') p++;
        }

        p = skip_val(p, end);
        count++;

        p = skip_ws(p, end);
        if (p < end && *p == ',') {
            p++;
        }
    }
    return count;
}

StrView cp_json_as_view(cp_json_val_t val) {
    if (val.type == CP_JSON_STRING && val.raw.len >= 2 && val.raw.data[0] == '"' && val.raw.data[val.raw.len - 1] == '"') {
        return cp_sv_from_parts(val.raw.data + 1, val.raw.len - 2);
    }
    return val.raw;
}

i64 cp_json_as_int(cp_json_val_t val, i64 default_val) {
    if (val.type != CP_JSON_NUMBER || val.raw.len == 0) return default_val;
    char buf[32];
    usize len = (val.raw.len < 31) ? val.raw.len : 31;
    memcpy(buf, val.raw.data, len);
    buf[len] = '\0';
    return (i64)strtoll(buf, NULL, 10);
}

f64 cp_json_as_float(cp_json_val_t val, f64 default_val) {
    if (val.type != CP_JSON_NUMBER || val.raw.len == 0) return default_val;
    char buf[64];
    usize len = (val.raw.len < 63) ? val.raw.len : 63;
    memcpy(buf, val.raw.data, len);
    buf[len] = '\0';
    return strtod(buf, NULL);
}

bool cp_json_as_bool(cp_json_val_t val, bool default_val) {
    if (val.type != CP_JSON_BOOL || val.raw.len == 0) return default_val;
    return (val.raw.len == 4 && memcmp(val.raw.data, "true", 4) == 0);
}

bool cp_json_is_null(cp_json_val_t val) {
    return val.type == CP_JSON_NULL;
}
