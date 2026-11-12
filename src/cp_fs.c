/**
 * @file cp_fs.c
 * @brief High-performance implementation of Filesystem and Path operations.
 */

#include "cplus/cp_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

/* -------------------------------------------------------------------------
 * Filesystem Operations
 * ------------------------------------------------------------------------- */
cp_strview_t cp_fs_read(cp_arena_t *arena, const char *path) {
    if (!path) return (cp_strview_t){ NULL, 0 };
    if (!arena) arena = Scratch();

    FILE *f = fopen(path, "rb");
    if (!f) return (cp_strview_t){ NULL, 0 };

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return (cp_strview_t){ NULL, 0 };
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return (cp_strview_t){ NULL, 0 };
    }
    fseek(f, 0, SEEK_SET);

    char *buf = (char *)ArenaAlloc(arena, (usize)size + 1);
    if (!buf) {
        fclose(f);
        return (cp_strview_t){ NULL, 0 };
    }

    usize bytes_read = fread(buf, 1, (usize)size, f);
    fclose(f);

    buf[bytes_read] = '\0';
    return cp_sv_from_parts(buf, bytes_read);
}

b32 cp_fs_write(const char *path, const void *data, usize len) {
    if (!path) return false;
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    if (len > 0 && data) {
        usize written = fwrite(data, 1, len, f);
        fclose(f);
        return written == len;
    }

    fclose(f);
    return true;
}

b32 cp_fs_append(const char *path, const void *data, usize len) {
    if (!path) return false;
    FILE *f = fopen(path, "ab");
    if (!f) return false;

    if (len > 0 && data) {
        usize written = fwrite(data, 1, len, f);
        fclose(f);
        return written == len;
    }

    fclose(f);
    return true;
}

b32 cp_fs_exists(const char *path) {
    if (!path) return false;
    return access(path, F_OK) == 0;
}

b32 cp_fs_is_dir(const char *path) {
    if (!path) return false;
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

usize cp_fs_size(const char *path) {
    if (!path) return 0;
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return (usize)st.st_size;
}

b32 cp_fs_remove(const char *path) {
    if (!path) return false;
    return remove(path) == 0;
}

b32 cp_fs_mkdir(const char *path) {
    if (!path) return false;
    return mkdir(path, 0755) == 0;
}

cp_strview_t *cp_fs_list_dir(cp_arena_t *arena, const char *path, usize *out_count) {
    if (!path) {
        if (out_count) *out_count = 0;
        return NULL;
    }
    if (!arena) arena = Scratch();

    DIR *dir = opendir(path);
    if (!dir) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    /* Count items */
    usize count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        count++;
    }
    rewinddir(dir);

    cp_strview_t *results = (cp_strview_t *)ArenaAlloc(arena, count * sizeof(cp_strview_t));
    usize idx = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        usize name_len = strlen(entry->d_name);
        char *copy = (char *)ArenaAlloc(arena, name_len + 1);
        memcpy(copy, entry->d_name, name_len);
        copy[name_len] = '\0';
        results[idx++] = cp_sv_from_parts(copy, name_len);
    }
    closedir(dir);

    if (out_count) *out_count = count;
    return results;
}

/* -------------------------------------------------------------------------
 * Path Manipulation Operations
 * ------------------------------------------------------------------------- */
cp_strview_t cp_path_join(cp_arena_t *arena, cp_strview_t a, cp_strview_t b) {
    if (!arena) arena = Scratch();

    while (a.len > 0 && a.data[a.len - 1] == '/') {
        a.len--;
    }
    while (b.len > 0 && b.data[0] == '/') {
        b.data++;
        b.len--;
    }

    usize total_len = a.len + 1 + b.len;
    char *buf = (char *)ArenaAlloc(arena, total_len + 1);
    memcpy(buf, a.data, a.len);
    buf[a.len] = '/';
    memcpy(buf + a.len + 1, b.data, b.len);
    buf[total_len] = '\0';

    return cp_sv_from_parts(buf, total_len);
}

cp_strview_t cp_path_ext(cp_strview_t path) {
    for (usize i = path.len; i > 0; i--) {
        if (path.data[i - 1] == '.') {
            return cp_sv_sub(path, i - 1, path.len - (i - 1));
        }
        if (path.data[i - 1] == '/') {
            break;
        }
    }
    return (cp_strview_t){ NULL, 0 };
}

cp_strview_t cp_path_base(cp_strview_t path) {
    for (usize i = path.len; i > 0; i--) {
        if (path.data[i - 1] == '/') {
            return cp_sv_sub(path, i, path.len - i);
        }
    }
    return path;
}

cp_strview_t cp_path_dir(cp_arena_t *arena, cp_strview_t path) {
    if (!arena) arena = Scratch();
    for (usize i = path.len; i > 0; i--) {
        if (path.data[i - 1] == '/') {
            return cp_sv_sub(path, 0, i - 1);
        }
    }
    return cp_sv_from_c(".");
}
