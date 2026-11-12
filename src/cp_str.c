/**
 * @file cp_str.c
 * @brief Implementation of StringView operations and Arena-backed String Builder.
 */

#include "cplus/cp_str.h"
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* -------------------------------------------------------------------------
 * String View Implementation
 * ------------------------------------------------------------------------- */
cp_strview_t cp_sv_from_c(const char *cstr) {
    if (!cstr) return (cp_strview_t){ NULL, 0 };
    return (cp_strview_t){ cstr, strlen(cstr) };
}

cp_strview_t cp_sv_from_parts(const char *data, usize len) {
    return (cp_strview_t){ data, len };
}

cp_strview_t cp_sv_sub(cp_strview_t sv, usize start, usize len) {
    if (start >= sv.len) {
        return (cp_strview_t){ NULL, 0 };
    }
    if (start + len > sv.len) {
        len = sv.len - start;
    }
    return (cp_strview_t){ sv.data + start, len };
}

cp_strview_t cp_sv_trim_left(cp_strview_t sv) {
    while (sv.len > 0 && isspace((unsigned char)*sv.data)) {
        sv.data++;
        sv.len--;
    }
    return sv;
}

cp_strview_t cp_sv_trim_right(cp_strview_t sv) {
    while (sv.len > 0 && isspace((unsigned char)sv.data[sv.len - 1])) {
        sv.len--;
    }
    return sv;
}

cp_strview_t cp_sv_trim(cp_strview_t sv) {
    return cp_sv_trim_right(cp_sv_trim_left(sv));
}

b32 cp_sv_equals(cp_strview_t a, cp_strview_t b) {
    if (a.len != b.len) return false;
    if (a.len == 0) return true;
    return memcmp(a.data, b.data, a.len) == 0;
}

b32 cp_sv_equals_c(cp_strview_t a, const char *cstr) {
    if (!cstr) return a.len == 0;
    usize clen = strlen(cstr);
    if (a.len != clen) return false;
    return memcmp(a.data, cstr, clen) == 0;
}

b32 cp_sv_starts_with(cp_strview_t sv, cp_strview_t prefix) {
    if (prefix.len > sv.len) return false;
    return memcmp(sv.data, prefix.data, prefix.len) == 0;
}

b32 cp_sv_ends_with(cp_strview_t sv, cp_strview_t suffix) {
    if (suffix.len > sv.len) return false;
    return memcmp(sv.data + (sv.len - suffix.len), suffix.data, suffix.len) == 0;
}

isize cp_sv_find(cp_strview_t sv, cp_strview_t needle) {
    if (needle.len == 0) return 0;
    if (needle.len > sv.len) return -1;

    for (usize i = 0; i <= sv.len - needle.len; i++) {
        if (memcmp(sv.data + i, needle.data, needle.len) == 0) {
            return (isize)i;
        }
    }
    return -1;
}

isize cp_sv_find_char(cp_strview_t sv, char c) {
    for (usize i = 0; i < sv.len; i++) {
        if (sv.data[i] == c) {
            return (isize)i;
        }
    }
    return -1;
}

usize cp_sv_split(cp_strview_t sv, char delimiter, cp_strview_t *out_views, usize max_views) {
    if (max_views == 0 || !out_views) return 0;

    usize count = 0;
    usize start = 0;

    for (usize i = 0; i < sv.len; i++) {
        if (sv.data[i] == delimiter) {
            if (count < max_views) {
                out_views[count++] = cp_sv_from_parts(sv.data + start, i - start);
            }
            start = i + 1;
        }
    }

    if (count < max_views) {
        out_views[count++] = cp_sv_from_parts(sv.data + start, sv.len - start);
    }

    return count;
}

cp_strview_t *cp_sv_split_arena(cp_strview_t sv, char delimiter, cp_arena_t *arena, usize *out_count) {
    if (!arena) arena = Scratch();

    /* Count items */
    usize count = 1;
    for (usize i = 0; i < sv.len; i++) {
        if (sv.data[i] == delimiter) count++;
    }

    cp_strview_t *result = (cp_strview_t *)ArenaAlloc(arena, count * sizeof(cp_strview_t));
    usize populated = cp_sv_split(sv, delimiter, result, count);
    if (out_count) *out_count = populated;
    return result;
}

/* -------------------------------------------------------------------------
 * Dynamic String / String Builder Implementation
 * ------------------------------------------------------------------------- */
static void _cp_str_ensure_cap(cp_string_t *str, usize additional) {
    if (str->len + additional + 1 <= str->cap) return;

    usize new_cap = (str->cap * 2) + additional + 16;
    char *new_data = (char *)ArenaAlloc(str->arena, new_cap);
    if (str->len > 0 && str->data) {
        memcpy(new_data, str->data, str->len);
    }
    str->data = new_data;
    str->cap = new_cap;
}

cp_string_t cp_str_new(cp_arena_t *arena, usize initial_capacity) {
    if (!arena) arena = Scratch();
    if (initial_capacity < 16) initial_capacity = 16;

    cp_string_t s;
    s.arena = arena;
    s.len = 0;
    s.cap = initial_capacity;
    s.data = (char *)ArenaAlloc(arena, initial_capacity);
    s.data[0] = '\0';
    return s;
}

cp_string_t cp_str_from_sv(cp_arena_t *arena, cp_strview_t sv) {
    cp_string_t s = cp_str_new(arena, sv.len + 1);
    cp_str_append_sv(&s, sv);
    return s;
}

cp_string_t cp_str_from_c(cp_arena_t *arena, const char *cstr) {
    return cp_str_from_sv(arena, cp_sv_from_c(cstr));
}

void cp_str_append_char(cp_string_t *str, char c) {
    _cp_str_ensure_cap(str, 1);
    str->data[str->len++] = c;
    str->data[str->len] = '\0';
}

void cp_str_append_sv(cp_string_t *str, cp_strview_t sv) {
    if (sv.len == 0) return;
    _cp_str_ensure_cap(str, sv.len);
    memcpy(str->data + str->len, sv.data, sv.len);
    str->len += sv.len;
    str->data[str->len] = '\0';
}

void cp_str_append_c(cp_string_t *str, const char *cstr) {
    if (!cstr) return;
    cp_str_append_sv(str, cp_sv_from_c(cstr));
}

void cp_str_append_f(cp_string_t *str, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    if (needed > 0) {
        _cp_str_ensure_cap(str, (usize)needed);
        vsnprintf(str->data + str->len, str->cap - str->len, fmt, args);
        str->len += (usize)needed;
    }
    va_end(args);
}

cp_string_t cp_str_format(cp_arena_t *arena, const char *fmt, ...) {
    if (!arena) arena = Scratch();

    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    cp_string_t s = cp_str_new(arena, (usize)needed + 1);
    if (needed > 0) {
        vsnprintf(s.data, s.cap, fmt, args);
        s.len = (usize)needed;
    }
    va_end(args);
    return s;
}

cp_string_t cp_str_format_scratch(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    cp_string_t s = cp_str_new(Scratch(), (usize)needed + 1);
    if (needed > 0) {
        vsnprintf(s.data, s.cap, fmt, args);
        s.len = (usize)needed;
    }
    va_end(args);
    return s;
}

cp_strview_t cp_str_to_sv(const cp_string_t *str) {
    if (!str || !str->data) return (cp_strview_t){ NULL, 0 };
    return (cp_strview_t){ str->data, str->len };
}

cp_string_t cp_str_join(cp_arena_t *arena, const cp_strview_t *views, usize count, cp_strview_t separator) {
    if (count == 0) return cp_str_new(arena, 16);

    usize total_len = 0;
    for (usize i = 0; i < count; i++) {
        total_len += views[i].len;
    }
    total_len += (count - 1) * separator.len;

    cp_string_t s = cp_str_new(arena, total_len + 1);
    for (usize i = 0; i < count; i++) {
        if (i > 0 && separator.len > 0) {
            cp_str_append_sv(&s, separator);
        }
        cp_str_append_sv(&s, views[i]);
    }
    return s;
}

cp_string_t cp_str_replace(cp_arena_t *arena, cp_strview_t src, cp_strview_t target, cp_strview_t replacement) {
    if (target.len == 0) return cp_str_from_sv(arena, src);

    cp_string_t s = cp_str_new(arena, src.len + 16);
    usize cursor = 0;

    while (cursor < src.len) {
        cp_strview_t remaining = cp_sv_sub(src, cursor, src.len - cursor);
        isize match_idx = cp_sv_find(remaining, target);
        if (match_idx == -1) {
            cp_str_append_sv(&s, remaining);
            break;
        }

        /* Append segment prior to match */
        cp_str_append_sv(&s, cp_sv_sub(remaining, 0, (usize)match_idx));
        /* Append replacement */
        cp_str_append_sv(&s, replacement);
        cursor += (usize)match_idx + target.len;
    }

    return s;
}

/* -------------------------------------------------------------------------
 * Base64 Encoding & Decoding Implementation
 * ------------------------------------------------------------------------- */
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const i8 b64_rev_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,  0, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

i64 cp_base64_encode(const void *src, usize src_len, char *dst, usize dst_cap) {
    if (!src && src_len > 0) return -1;
    usize out_len = ((src_len + 2) / 3) * 4;
    if (!dst || dst_cap <= out_len) return (i64)out_len;

    const u8 *in = (const u8 *)src;
    usize i = 0, j = 0;
    while (i + 3 <= src_len) {
        u32 octet_a = in[i];
        u32 octet_b = in[i + 1];
        u32 octet_c = in[i + 2];
        i += 3;

        u32 triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        u8 c0 = (u8)b64_table[(triple >> 18) & 0x3F];
        u8 c1 = (u8)b64_table[(triple >> 12) & 0x3F];
        u8 c2 = (u8)b64_table[(triple >> 6) & 0x3F];
        u8 c3 = (u8)b64_table[triple & 0x3F];

        u32 word = (u32)c0 | ((u32)c1 << 8) | ((u32)c2 << 16) | ((u32)c3 << 24);
        memcpy(&dst[j], &word, 4);
        j += 4;
    }

    if (i < src_len) {
        usize rem = src_len - i;
        if (rem == 1) {
            u32 octet_a = in[i];
            u32 triple = (octet_a << 16);
            dst[j++] = b64_table[(triple >> 18) & 0x3F];
            dst[j++] = b64_table[(triple >> 12) & 0x3F];
            dst[j++] = '=';
            dst[j++] = '=';
        } else if (rem == 2) {
            u32 octet_a = in[i];
            u32 octet_b = in[i + 1];
            u32 triple = (octet_a << 16) | (octet_b << 8);
            dst[j++] = b64_table[(triple >> 18) & 0x3F];
            dst[j++] = b64_table[(triple >> 12) & 0x3F];
            dst[j++] = b64_table[(triple >> 6) & 0x3F];
            dst[j++] = '=';
        }
    }

    dst[j] = '\0';
    return (i64)j;
}

i64 cp_base64_decode(const char *src, usize src_len, void *dst, usize dst_cap) {
    if (!src && src_len > 0) return -1;
    if (src_len == 0) return 0;
    if (src_len % 4 != 0) return -1;

    usize pad = 0;
    if (src[src_len - 1] == '=') pad++;
    if (src_len >= 2 && src[src_len - 2] == '=') pad++;

    usize out_len = (src_len / 4) * 3 - pad;
    if (!dst || dst_cap < out_len) return (i64)out_len;

    u8 *out = (u8 *)dst;
    usize j = 0;
    for (usize i = 0; i < src_len; i += 4) {
        i8 b0 = b64_rev_table[(u8)src[i]];
        i8 b1 = b64_rev_table[(u8)src[i+1]];
        i8 b2 = (src[i+2] == '=') ? 0 : b64_rev_table[(u8)src[i+2]];
        i8 b3 = (src[i+3] == '=') ? 0 : b64_rev_table[(u8)src[i+3]];

        if (b0 < 0 || b1 < 0 || (src[i+2] != '=' && b2 < 0) || (src[i+3] != '=' && b3 < 0)) {
            return -1;
        }

        u32 triple = ((u32)b0 << 18) | ((u32)b1 << 12) | ((u32)b2 << 6) | (u32)b3;
        if (j < out_len) out[j++] = (triple >> 16) & 0xFF;
        if (j < out_len) out[j++] = (triple >> 8) & 0xFF;
        if (j < out_len) out[j++] = triple & 0xFF;
    }

    return (i64)j;
}

cp_strview_t cp_base64_encode_arena(const void *src, usize src_len, cp_arena_t *arena) {
    if (!arena) arena = Scratch();
    usize needed = ((src_len + 2) / 3) * 4;
    char *buf = (char *)ArenaAlloc(arena, needed + 1);
    i64 res = cp_base64_encode(src, src_len, buf, needed + 1);
    if (res < 0) return (cp_strview_t){ NULL, 0 };
    return (cp_strview_t){ buf, (usize)res };
}

void *cp_base64_decode_arena(const char *src, usize src_len, cp_arena_t *arena, usize *out_len) {
    if (!arena) arena = Scratch();
    if (src_len % 4 != 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    usize pad = 0;
    if (src_len > 0 && src[src_len - 1] == '=') pad++;
    if (src_len >= 2 && src[src_len - 2] == '=') pad++;
    usize max_len = (src_len / 4) * 3 - pad;

    u8 *buf = (u8 *)ArenaAlloc(arena, max_len > 0 ? max_len : 1);
    i64 res = cp_base64_decode(src, src_len, buf, max_len);
    if (res < 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    if (out_len) *out_len = (usize)res;
    return buf;
}
