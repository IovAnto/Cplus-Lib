/**
 * @file cp_map.c
 * @brief Implementation of open-addressing Hash Map with linear probing.
 */

#include "cplus/cp_map.h"
#include "cplus/cp_hash.h"
#include <string.h>
#include <stdlib.h>

static usize next_power_of_two(usize x) {
    if (x <= 16) return 16;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

static char *cp_map_dup_key(const char *key, cp_arena_t *arena) {
    if (!key) return NULL;
    usize len = strlen(key);
    char *copy;
    if (arena) {
        copy = (char *)ArenaAlloc(arena, len + 1);
    } else {
        copy = (char *)malloc(len + 1);
    }
    if (copy) {
        memcpy(copy, key, len + 1);
    }
    return copy;
}

cp_map_t *cp_map_create(usize initial_capacity, cp_arena_t *arena) {
    usize cap = next_power_of_two(initial_capacity);
    cp_map_t *map;
    if (arena) {
        map = (cp_map_t *)ArenaAlloc(arena, sizeof(cp_map_t));
        map->entries = (cp_map_entry_t *)ArenaAlloc(arena, cap * sizeof(cp_map_entry_t));
    } else {
        map = (cp_map_t *)calloc(1, sizeof(cp_map_t));
        if (!map) return NULL;
        map->entries = (cp_map_entry_t *)calloc(cap, sizeof(cp_map_entry_t));
        if (!map->entries) {
            free(map);
            return NULL;
        }
    }
    memset(map->entries, 0, cap * sizeof(cp_map_entry_t));
    map->capacity = cap;
    map->count = 0;
    map->tombstones = 0;
    map->arena = arena;
    return map;
}

void cp_map_destroy(cp_map_t *map) {
    if (!map) return;
    if (!map->arena) {
        for (usize i = 0; i < map->capacity; ++i) {
            if (map->entries[i].occupied && !map->entries[i].deleted) {
                free(map->entries[i].key);
            }
        }
        free(map->entries);
        free(map);
    }
}

static bool cp_map_resize(cp_map_t *map, usize new_cap) {
    cp_map_entry_t *old_entries = map->entries;
    usize old_cap = map->capacity;

    cp_map_entry_t *new_entries;
    if (map->arena) {
        new_entries = (cp_map_entry_t *)ArenaAlloc(map->arena, new_cap * sizeof(cp_map_entry_t));
    } else {
        new_entries = (cp_map_entry_t *)calloc(new_cap, sizeof(cp_map_entry_t));
    }
    if (!new_entries) return false;
    memset(new_entries, 0, new_cap * sizeof(cp_map_entry_t));

    usize mask = new_cap - 1;
    for (usize i = 0; i < old_cap; ++i) {
        if (old_entries[i].occupied && !old_entries[i].deleted) {
            usize idx = old_entries[i].hash & mask;
            while (new_entries[idx].occupied) {
                idx = (idx + 1) & mask;
            }
            new_entries[idx] = old_entries[i];
        }
    }

    if (!map->arena) {
        free(old_entries);
    }

    map->entries = new_entries;
    map->capacity = new_cap;
    map->tombstones = 0;
    return true;
}

bool cp_map_set(cp_map_t *map, const char *key, void *value) {
    if (!map || !key) return false;

    /* Resize when load factor > 70% */
    if ((map->count + map->tombstones + 1) * 10 >= map->capacity * 7) {
        usize new_cap = map->capacity * 2;
        if (!cp_map_resize(map, new_cap)) return false;
    }

    u64 hash = Hash64(key, strlen(key));
    usize mask = map->capacity - 1;
    usize idx = hash & mask;
    usize tombstone_idx = (usize)-1;

    while (map->entries[idx].occupied) {
        if (map->entries[idx].deleted) {
            if (tombstone_idx == (usize)-1) {
                tombstone_idx = idx;
            }
        } else if (map->entries[idx].hash == hash && strcmp(map->entries[idx].key, key) == 0) {
            /* Update existing key value */
            map->entries[idx].value = value;
            return true;
        }
        idx = (idx + 1) & mask;
    }

    /* Reuse tombstone if found, else use first empty slot */
    usize insert_idx = (tombstone_idx != (usize)-1) ? tombstone_idx : idx;
    char *k = cp_map_dup_key(key, map->arena);
    if (!k) return false;

    if (map->entries[insert_idx].deleted) {
        map->tombstones--;
    }
    map->entries[insert_idx].key = k;
    map->entries[insert_idx].value = value;
    map->entries[insert_idx].hash = hash;
    map->entries[insert_idx].occupied = true;
    map->entries[insert_idx].deleted = false;
    map->count++;
    return true;
}

void *cp_map_get(const cp_map_t *map, const char *key) {
    if (!map || !key || map->count == 0) return NULL;

    u64 hash = Hash64(key, strlen(key));
    usize mask = map->capacity - 1;
    usize idx = hash & mask;

    while (map->entries[idx].occupied) {
        if (!map->entries[idx].deleted && map->entries[idx].hash == hash && strcmp(map->entries[idx].key, key) == 0) {
            return map->entries[idx].value;
        }
        idx = (idx + 1) & mask;
    }
    return NULL;
}

bool cp_map_has(const cp_map_t *map, const char *key) {
    return cp_map_get(map, key) != NULL;
}

bool cp_map_delete(cp_map_t *map, const char *key) {
    if (!map || !key || map->count == 0) return false;

    u64 hash = Hash64(key, strlen(key));
    usize mask = map->capacity - 1;
    usize idx = hash & mask;

    while (map->entries[idx].occupied) {
        if (!map->entries[idx].deleted && map->entries[idx].hash == hash && strcmp(map->entries[idx].key, key) == 0) {
            if (!map->arena) {
                free(map->entries[idx].key);
            }
            map->entries[idx].key = NULL;
            map->entries[idx].value = NULL;
            map->entries[idx].deleted = true;
            map->count--;
            map->tombstones++;
            return true;
        }
        idx = (idx + 1) & mask;
    }
    return false;
}

void cp_map_clear(cp_map_t *map) {
    if (!map) return;
    for (usize i = 0; i < map->capacity; ++i) {
        if (map->entries[i].occupied && !map->entries[i].deleted) {
            if (!map->arena) {
                free(map->entries[i].key);
            }
        }
    }
    memset(map->entries, 0, map->capacity * sizeof(cp_map_entry_t));
    map->count = 0;
    map->tombstones = 0;
}

usize cp_map_len(const cp_map_t *map) {
    return map ? map->count : 0;
}
