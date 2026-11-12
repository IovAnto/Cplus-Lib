/**
 * @file cp_str.h
 * @brief Zero-allocation StringView and high-performance Arena-backed String Builder.
 */

#ifndef CPLUS_STR_H
#define CPLUS_STR_H

#include "cp_core.h"
#include "cp_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * String View (Zero-allocation slice)
 * ------------------------------------------------------------------------- */
typedef struct cp_strview {
    const char *data;
    usize len;
} cp_strview_t;

typedef cp_strview_t StrView;

#define StrViewLit(str_literal) ((cp_strview_t){ (str_literal), sizeof(str_literal) - 1 })
#define S(str_literal)          StrViewLit(str_literal)

cp_strview_t cp_sv_from_c(const char *cstr);
cp_strview_t cp_sv_from_parts(const char *data, usize len);
cp_strview_t cp_sv_sub(cp_strview_t sv, usize start, usize len);
cp_strview_t cp_sv_trim_left(cp_strview_t sv);
cp_strview_t cp_sv_trim_right(cp_strview_t sv);
cp_strview_t cp_sv_trim(cp_strview_t sv);

b32 cp_sv_equals(cp_strview_t a, cp_strview_t b);
b32 cp_sv_equals_c(cp_strview_t a, const char *cstr);
b32 cp_sv_starts_with(cp_strview_t sv, cp_strview_t prefix);
b32 cp_sv_ends_with(cp_strview_t sv, cp_strview_t suffix);
isize cp_sv_find(cp_strview_t sv, cp_strview_t needle);
isize cp_sv_find_char(cp_strview_t sv, char c);

/* Splitting into array of StrView on arena */
usize cp_sv_split(cp_strview_t sv, char delimiter, cp_strview_t *out_views, usize max_views);
cp_strview_t *cp_sv_split_arena(cp_strview_t sv, char delimiter, cp_arena_t *arena, usize *out_count);

/* -------------------------------------------------------------------------
 * Dynamic String / String Builder
 * ------------------------------------------------------------------------- */
typedef struct cp_string {
    char *data;
    usize len;
    usize cap;
    cp_arena_t *arena;
} cp_string_t;

typedef cp_string_t Str;
typedef cp_string_t String;

cp_string_t cp_str_new(cp_arena_t *arena, usize initial_capacity);
cp_string_t cp_str_from_sv(cp_arena_t *arena, cp_strview_t sv);
cp_string_t cp_str_from_c(cp_arena_t *arena, const char *cstr);

void cp_str_append_char(cp_string_t *str, char c);
void cp_str_append_sv(cp_string_t *str, cp_strview_t sv);
void cp_str_append_c(cp_string_t *str, const char *cstr);
void cp_str_append_f(cp_string_t *str, const char *fmt, ...);

cp_string_t cp_str_format(cp_arena_t *arena, const char *fmt, ...);
cp_string_t cp_str_format_scratch(const char *fmt, ...);

cp_strview_t cp_str_to_sv(const cp_string_t *str);
cp_string_t cp_str_join(cp_arena_t *arena, const cp_strview_t *views, usize count, cp_strview_t separator);
cp_string_t cp_str_replace(cp_arena_t *arena, cp_strview_t src, cp_strview_t target, cp_strview_t replacement);

/* -------------------------------------------------------------------------
 * Base64 Encoding & Decoding
 * ------------------------------------------------------------------------- */
i64 cp_base64_encode(const void *src, usize src_len, char *dst, usize dst_cap);
i64 cp_base64_decode(const char *src, usize src_len, void *dst, usize dst_cap);
cp_strview_t cp_base64_encode_arena(const void *src, usize src_len, cp_arena_t *arena);
void *cp_base64_decode_arena(const char *src, usize src_len, cp_arena_t *arena, usize *out_len);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define StrFromC(cstr)                 cp_sv_from_c(cstr)
#define StrFromParts(data, len)        cp_sv_from_parts(data, len)
#define StrSub(sv, start, len)         cp_sv_sub(sv, start, len)
#define StrTrim(sv)                    cp_sv_trim(sv)
#define StrTrimLeft(sv)                cp_sv_trim_left(sv)
#define StrTrimRight(sv)               cp_sv_trim_right(sv)

#define StrEquals(a, b)                cp_sv_equals(a, b)
#define StrEqualsC(a, cstr)            cp_sv_equals_c(a, cstr)
#define StrStartsWith(sv, prefix)      cp_sv_starts_with(sv, prefix)
#define StrEndsWith(sv, suffix)        cp_sv_ends_with(sv, suffix)
#define StrFind(sv, needle)            cp_sv_find(sv, needle)
#define StrFindChar(sv, c)             cp_sv_find_char(sv, c)

#define StrSplit(sv, delim, out, max)  cp_sv_split(sv, delim, out, max)
#define StrSplitArena(sv, d, a, cnt)   cp_sv_split_arena(sv, d, a, cnt)

#define StrNew(arena, cap)             cp_str_new(arena, cap)
#define StrLen(str)                    ((str)->len)
#define StrAppend(str, sv)             cp_str_append_sv(str, sv)
#define StrAppendC(str, cstr)          cp_str_append_c(str, cstr)
#define StrFormat(arena, fmt, ...)     cp_str_format(arena, fmt, __VA_ARGS__)
#define StrFormatScratch(fmt, ...)     cp_str_format_scratch(fmt, __VA_ARGS__)
#define StrJoin(arena, v, cnt, sep)    cp_str_join(arena, v, cnt, sep)
#define StrReplace(arena, s, t, r)     cp_str_replace(arena, s, t, r)

#define Base64Encode(src, len, dst, cap)    cp_base64_encode(src, len, dst, cap)
#define Base64Decode(src, len, dst, cap)    cp_base64_decode(src, len, dst, cap)
#define Base64EncodeArena(src, len, arena)  cp_base64_encode_arena(src, len, arena)
#define Base64DecodeArena(src, l, a, out_l) cp_base64_decode_arena(src, l, a, out_l)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_strview_t (*const fromC)(const char *cstr);
    cp_strview_t (*const sub)(cp_strview_t sv, usize start, usize len);
    cp_strview_t (*const trim)(cp_strview_t sv);
    b32 (*const equals)(cp_strview_t a, cp_strview_t b);
    b32 (*const startsWith)(cp_strview_t sv, cp_strview_t prefix);
    b32 (*const endsWith)(cp_strview_t sv, cp_strview_t suffix);
    isize (*const find)(cp_strview_t sv, cp_strview_t needle);
    cp_strview_t *(*const split)(cp_strview_t sv, char delimiter, cp_arena_t *arena, usize *out_count);
    cp_string_t (*const format)(cp_arena_t *arena, const char *fmt, ...);
    cp_string_t (*const join)(cp_arena_t *arena, const cp_strview_t *views, usize count, cp_strview_t separator);
    cp_string_t (*const replace)(cp_arena_t *arena, cp_strview_t src, cp_strview_t target, cp_strview_t replacement);
} cp_str_namespace_t;

static const cp_str_namespace_t str = {
    .fromC = cp_sv_from_c,
    .sub = cp_sv_sub,
    .trim = cp_sv_trim,
    .equals = cp_sv_equals,
    .startsWith = cp_sv_starts_with,
    .endsWith = cp_sv_ends_with,
    .find = cp_sv_find,
    .split = cp_sv_split_arena,
    .format = cp_str_format,
    .join = cp_str_join,
    .replace = cp_str_replace,
};

typedef struct {
    i64 (*const encode)(const void *src, usize src_len, char *dst, usize dst_cap);
    i64 (*const decode)(const char *src, usize src_len, void *dst, usize dst_cap);
    cp_strview_t (*const encodeArena)(const void *src, usize src_len, cp_arena_t *arena);
    void *(*const decodeArena)(const char *src, usize src_len, cp_arena_t *arena, usize *out_len);
} cp_base64_namespace_t;

static const cp_base64_namespace_t base64 = {
    .encode = cp_base64_encode,
    .decode = cp_base64_decode,
    .encodeArena = cp_base64_encode_arena,
    .decodeArena = cp_base64_decode_arena,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_STR_H */
