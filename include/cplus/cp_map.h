/**
 * @file cp_map.h
 * @brief High-performance, open-addressing Hash Map with linear probing and optional Arena backing.
 */

#ifndef CPLUS_MAP_H
#define CPLUS_MAP_H

#include "cp_core.h"
#include "cp_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Hash Map Data Structures
 * ------------------------------------------------------------------------- */
typedef struct {
    char *key;
    void *value;
    u64 hash;
    bool occupied;
    bool deleted;
} cp_map_entry_t;

typedef cp_map_entry_t MapEntry;

typedef struct cp_map {
    cp_map_entry_t *entries;
    usize capacity;
    usize count;
    usize tombstones;
    cp_arena_t *arena;
} cp_map_t;

typedef cp_map_t HashMap;

/* -------------------------------------------------------------------------
 * Core Map Operations
 * ------------------------------------------------------------------------- */
cp_map_t *cp_map_create(usize initial_capacity, cp_arena_t *arena);
void cp_map_destroy(cp_map_t *map);

bool cp_map_set(cp_map_t *map, const char *key, void *value);
void *cp_map_get(const cp_map_t *map, const char *key);
bool cp_map_has(const cp_map_t *map, const char *key);
bool cp_map_delete(cp_map_t *map, const char *key);
void cp_map_clear(cp_map_t *map);
usize cp_map_len(const cp_map_t *map);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define MapCreate(cap, arena)           cp_map_create(cap, arena)
#define MapDestroy(map)                 cp_map_destroy(map)
#define MapSet(map, key, val)           cp_map_set(map, key, (void *)(val))
#define MapGet(map, key)                cp_map_get(map, key)
#define MapGetT(map, key, type)         ((type)cp_map_get(map, key))
#define MapHas(map, key)                cp_map_has(map, key)
#define MapDelete(map, key)             cp_map_delete(map, key)
#define MapClear(map)                   cp_map_clear(map)
#define MapLen(map)                     cp_map_len(map)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_map_t *(*const create)(usize initial_capacity, cp_arena_t *arena);
    void (*const destroy)(cp_map_t *map);
    bool (*const set)(cp_map_t *map, const char *key, void *value);
    void *(*const get)(const cp_map_t *map, const char *key);
    bool (*const has)(const cp_map_t *map, const char *key);
    bool (*const delete_entry)(cp_map_t *map, const char *key);
    void (*const clear)(cp_map_t *map);
    usize (*const len)(const cp_map_t *map);
} cp_map_namespace_t;

static const cp_map_namespace_t map = {
    .create = cp_map_create,
    .destroy = cp_map_destroy,
    .set = cp_map_set,
    .get = cp_map_get,
    .has = cp_map_has,
    .delete_entry = cp_map_delete,
    .clear = cp_map_clear,
    .len = cp_map_len,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_MAP_H */
