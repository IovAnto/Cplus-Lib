# Module Specification: cp_fs (High-Throughput Filesystem Subsystem)

## 1. TL;DR
The `cp_fs` module provides atomic file reads into linear `Arena` memory, atomic file writes, directory scanning, and path manipulation utilities with zero libc heap fragmentation.

```c
#include "cplus/cplus.h"

Arena *arena = ArenaCreate(64 * 1024);

/* Atomic Read into Arena Memory */
StrView content = FileRead(arena, "/etc/hosts");
if (content.data != NULL) {
    printf("Read %zu bytes from /etc/hosts\n", content.len);
}

/* Atomic File Write */
FileWrite("/tmp/config.cfg", "timeout=30\n", 11);

ArenaDestroy(arena);
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Atomic Read Engine (`FileRead`)
- Queries file size via POSIX `fstat()`.
- Allocates a contiguous block of `file_size + 1` bytes on the specified `Arena`.
- Reads the entire payload in a single `read()` system call, appending a trailing `\0` null byte.
- Returns a zero-allocation `StrView` referencing the arena buffer.
- `FileReadScratch(path)` reads the file into the thread-local scratch arena ring, ideal for temporary parsing.

### 2.2 Path Sanitization & Querying
- All path utilities (`PathExt`, `PathBase`, `PathDir`, `PathJoin`) work with `StrView` slices to avoid allocating temporary string buffers.

---

## 3. Operational Guarantees & Contracts

| Metric | FileRead | FileWrite | Path Operations |
|---|---|---|---|
| **I/O Complexity** | $O(N)$ file size (single system call) | $O(N)$ (single write system call) | $O(1)$ pointer math |
| **Memory Allocation** | Linear Arena allocation | 0 heap allocations | 0 heap allocations |
| **Null Termination** | Guaranteed (`data[len] = '\0'`) | Preserves binary bytes | Preserves slice boundaries |
| **Thread-Safety** | Safe (atomic file descriptor) | Safe (kernel write synchronization) | Pure functions |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Read to Arena | `FileRead(arena, path)` | `fs.read(arena, path)` | Read file into Arena buffer |
| Read to Scratch | `FileReadScratch(path)` | N/A | Read file into temporary scratch |
| Write Buffer | `FileWrite(path, data, len)` | `fs.write(path, data, len)` | Atomic file write / truncate |
| Write StrView | `FileWriteStr(path, strview)` | N/A | Write StrView slice directly to disk |
| Append Buffer | `FileAppend(path, data, len)` | `fs.append(...)` | Append bytes to file |
| Check Exists | `FileExists(path)` | `fs.exists(path)` | Check file existence (stat) |
| Check Is Dir | `FileIsDir(path)` | `fs.isDir(path)` | Check if path is a directory |
| File Size | `FileSize(path)` | `fs.size(path)` | Query file size in bytes |
| Delete File | `FileRemove(path)` | `fs.remove(path)` | Unlink file from filesystem |
| Create Dir | `FileMkdir(path)` | `fs.mkdir(path)` | Create single directory (0755) |
| List Directory | `FileListDir(arena, path, &cnt)`| `fs.listDir(...)` | Read directory entries into StrView array |
| Path Extension | `PathExt(path_sv)` | N/A | Extract file extension (e.g. ".log") |
| Path Basename | `PathBase(path_sv)` | N/A | Extract filename without directory |
| Path Directory | `PathDir(arena, path_sv)` | N/A | Extract parent directory path |
| Path Join | `PathJoin(arena, p1, p2)` | N/A | Join two paths with slash separator |

---

## 5. Use Cases
1. **Config File Ingestion**: Read JSON/YAML/INI configuration files into scratch memory in 1 call.
2. **Directory Walkers & Asset Loaders**: Iterate asset files in game or web engines using `FileListDir`.
3. **Atomic State Persistence**: Save telemetry and binary state to disk using `FileWrite`.

---

## 6. Complete C11 Usage Examples

### Example: Path Inspection & File Backup
```c
#include "cplus/cplus.h"

void backup_config(const char *source_path) {
    TempArena scratch = ScratchBegin();
    
    StrView path_sv = StrFromC(source_path);
    StrView filename = PathBase(path_sv);
    StrView backup_dir = S("/tmp/backups");
    
    FileMkdir("/tmp/backups");
    StrView target_path = PathJoin(scratch.arena, backup_dir, filename);
    
    StrView data = FileRead(scratch.arena, source_path);
    if (data.data != NULL) {
        FileWriteStr(target_path.data, data);
        printf("Backed up %zu bytes to %s\n", data.len, target_path.data);
    }
    
    ScratchEnd(scratch);
}
```

---

## 7. Limitations & Operational Constraints
- **File Size Clamping**: Files exceeding available system RAM cannot be mapped into an `Arena`. For gigabyte-scale datasets, use streaming reads or standard memory-mapped files.
- **Symlink Traversal**: `FileExists` uses `stat()`, which resolves symbolic links.
