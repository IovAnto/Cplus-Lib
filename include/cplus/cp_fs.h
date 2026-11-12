/**
 * @file cp_fs.h
 * @brief High-level, zero-latency Filesystem and Path operations.
 */

#ifndef CPLUS_FS_H
#define CPLUS_FS_H

#include "cp_core.h"
#include "cp_mem.h"
#include "cp_str.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Filesystem Operations
 * ------------------------------------------------------------------------- */
cp_strview_t cp_fs_read(cp_arena_t *arena, const char *path);
b32 cp_fs_write(const char *path, const void *data, usize len);
b32 cp_fs_append(const char *path, const void *data, usize len);

b32 cp_fs_exists(const char *path);
b32 cp_fs_is_dir(const char *path);
usize cp_fs_size(const char *path);
b32 cp_fs_remove(const char *path);
b32 cp_fs_mkdir(const char *path);

cp_strview_t *cp_fs_list_dir(cp_arena_t *arena, const char *path, usize *out_count);

/* -------------------------------------------------------------------------
 * Path Manipulation Operations
 * ------------------------------------------------------------------------- */
cp_strview_t cp_path_join(cp_arena_t *arena, cp_strview_t a, cp_strview_t b);
cp_strview_t cp_path_ext(cp_strview_t path);
cp_strview_t cp_path_base(cp_strview_t path);
cp_strview_t cp_path_dir(cp_arena_t *arena, cp_strview_t path);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define FileRead(arena, path)          cp_fs_read(arena, path)
#define FileReadScratch(path)          cp_fs_read(Scratch(), path)
#define FileWrite(path, data, len)     cp_fs_write(path, data, len)
#define FileWriteStr(path, strview)    cp_fs_write(path, (strview).data, (strview).len)
#define FileAppend(path, data, len)    cp_fs_append(path, data, len)
#define FileAppendStr(path, strview)   cp_fs_append(path, (strview).data, (strview).len)

#define FileExists(path)               cp_fs_exists(path)
#define FileIsDir(path)                cp_fs_is_dir(path)
#define FileSize(path)                 cp_fs_size(path)
#define FileRemove(path)               cp_fs_remove(path)
#define FileMkdir(path)                cp_fs_mkdir(path)
#define FileListDir(arena, path, count) cp_fs_list_dir(arena, path, count)

#define PathJoin(arena, a, b)          cp_path_join(arena, a, b)
#define PathExt(path)                  cp_path_ext(path)
#define PathBase(path)                 cp_path_base(path)
#define PathDir(arena, path)           cp_path_dir(arena, path)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_strview_t (*const read)(cp_arena_t *arena, const char *path);
    b32 (*const write)(const char *path, const void *data, usize len);
    b32 (*const append)(const char *path, const void *data, usize len);
    b32 (*const exists)(const char *path);
    b32 (*const isDir)(const char *path);
    usize (*const size)(const char *path);
    b32 (*const remove)(const char *path);
    b32 (*const mkdir)(const char *path);
    cp_strview_t *(*const listDir)(cp_arena_t *arena, const char *path, usize *out_count);
} cp_fs_namespace_t;

typedef struct {
    cp_strview_t (*const join)(cp_arena_t *arena, cp_strview_t a, cp_strview_t b);
    cp_strview_t (*const ext)(cp_strview_t path);
    cp_strview_t (*const base)(cp_strview_t path);
    cp_strview_t (*const dir)(cp_arena_t *arena, cp_strview_t path);
} cp_path_namespace_t;

static const cp_fs_namespace_t fs = {
    .read = cp_fs_read,
    .write = cp_fs_write,
    .append = cp_fs_append,
    .exists = cp_fs_exists,
    .isDir = cp_fs_is_dir,
    .size = cp_fs_size,
    .remove = cp_fs_remove,
    .mkdir = cp_fs_mkdir,
    .listDir = cp_fs_list_dir,
};

static const cp_path_namespace_t path = {
    .join = cp_path_join,
    .ext = cp_path_ext,
    .base = cp_path_base,
    .dir = cp_path_dir,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_FS_H */
