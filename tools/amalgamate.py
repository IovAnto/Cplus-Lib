#!/usr/bin/env python3
"""
tools/amalgamate.py
Combines modular Cplus headers and implementation files into:
1. dist/cplus.h (Unified single-header library with #define CPLUS_IMPLEMENTATION)
2. dist/standalone/*.h (Self-contained standalone single-header modules)
"""

import os
import re
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent.parent
INCLUDE_DIR = ROOT_DIR / "include" / "cplus"
SRC_DIR = ROOT_DIR / "src"
DIST_DIR = ROOT_DIR / "dist"
STANDALONE_DIR = DIST_DIR / "standalone"

MODULES = [
    "cp_core",
    "cp_mem",
    "cp_str",
    "cp_fs",
    "cp_ds",
    "cp_graph",
    "cp_algo",
    "cp_math",
    "cp_thread",
    "cp_net",
    "cp_tui",
    "cp_hash",
    "cp_map",
    "cp_args",
    "cp_time",
    "cp_json",
    "cp_stack",
    "cp_os",
]

def extract_system_includes(content: str):
    return set(re.findall(r'#include\s+<[^>]+>', content))

def strip_local_includes(content: str):
    # Remove #include "..." and #include <cplus/...>
    lines = []
    for line in content.splitlines():
        if re.match(r'^\s*#include\s+["<](?:cplus/)?cp_[^">]+[">]', line):
            continue
        if re.match(r'^\s*#include\s+"cplus\.h"', line):
            continue
        if re.match(r'^\s*#define\s+(_POSIX_C_SOURCE|_GNU_SOURCE)', line):
            continue
        lines.append(line)
    return "\n".join(lines)

def strip_header_guards(content: str):
    lines = content.splitlines()
    filtered = []
    for line in lines:
        if re.match(r'^\s*#ifndef\s+CPLUS_', line) or re.match(r'^\s*#define\s+CPLUS_', line):
            continue
        if re.match(r'^\s*#endif\s*/\*\s*CPLUS_', line):
            continue
        filtered.append(line)
    return "\n".join(filtered)

def strip_extern_c(content: str):
    lines = content.splitlines()
    filtered = []
    for line in lines:
        if 'extern "C"' in line or '#ifdef __cplusplus' in line:
            continue
        if line.strip() == '}' and len(filtered) > 0 and 'extern "C"' in content:
            # check if closing extern C
            pass
        filtered.append(line)
    return "\n".join(filtered)

def generate_amalgamated():
    DIST_DIR.mkdir(parents=True, exist_ok=True)
    out_file = DIST_DIR / "cplus.h"

    all_sys_includes = set()

    # Collect headers and sources
    headers_content = {}
    sources_content = {}

    for mod in MODULES:
        h_path = INCLUDE_DIR / f"{mod}.h"
        if h_path.exists():
            text = h_path.read_text(encoding="utf-8")
            all_sys_includes.update(extract_system_includes(text))
            headers_content[mod] = text

        s_path = SRC_DIR / f"{mod}.c"
        if s_path.exists():
            text = s_path.read_text(encoding="utf-8")
            all_sys_includes.update(extract_system_includes(text))
            sources_content[mod] = text

    sorted_sys_includes = sorted(list(all_sys_includes))

    out = []
    out.append("/**")
    out.append(" * @file cplus.h")
    out.append(" * @brief Cplus Amalgamated Single-Header Library")
    out.append(" * @version 0.1.0")
    out.append(" * @author Cplus Systems Architecture")
    out.append(" *")
    out.append(" * Modern Zero-Latency Foundation Library for C (C11).")
    out.append(" * Dual PascalCase and Dot-Notation API.")
    out.append(" *")
    out.append(" * USAGE:")
    out.append(" *   In exactly ONE compilation unit (.c file), add:")
    out.append(" *     #define CPLUS_IMPLEMENTATION")
    out.append(" *     #include \"cplus.h\"")
    out.append(" *")
    out.append(" *   In all other files:")
    out.append(" *     #include \"cplus.h\"")
    out.append(" */")
    out.append("")
    out.append("#ifndef CPLUS_AMALGAMATED_H")
    out.append("#define CPLUS_AMALGAMATED_H")
    out.append("")
    out.append("/* Platform & Standard Library Includes */")
    out.append("#ifndef _POSIX_C_SOURCE")
    out.append("#define _POSIX_C_SOURCE 200809L")
    out.append("#endif")
    out.append("#ifndef _GNU_SOURCE")
    out.append("#define _GNU_SOURCE")
    out.append("#endif")
    out.append("")
    for inc in sorted_sys_includes:
        out.append(inc)
    out.append("")
    out.append("#ifdef __cplusplus")
    out.append('extern "C" {')
    out.append("#endif")
    out.append("")
    out.append("/* =========================================================================")
    out.append(" * HEADER DECLARATIONS & INLINED TYPES")
    out.append(" * ========================================================================= */")
    out.append("")

    for mod in MODULES:
        if mod in headers_content:
            clean = strip_local_includes(headers_content[mod])
            clean = strip_header_guards(clean)
            out.append(f"/* --- Module: {mod} --- */")
            out.append(clean.strip())
            out.append("")

    out.append("#ifdef __cplusplus")
    out.append("}")
    out.append("#endif")
    out.append("")
    out.append("#endif /* CPLUS_AMALGAMATED_H */")
    out.append("")
    out.append("/* =========================================================================")
    out.append(" * IMPLEMENTATIONS (#define CPLUS_IMPLEMENTATION or #define CP_IMPLEMENTATION)")
    out.append(" * ========================================================================= */")
    out.append("#if defined(CPLUS_IMPLEMENTATION) || defined(CP_IMPLEMENTATION)")
    out.append("")
    out.append("#ifdef __cplusplus")
    out.append('extern "C" {')
    out.append("#endif")
    out.append("")

    for mod in MODULES:
        if mod in sources_content:
            clean = strip_local_includes(sources_content[mod])
            out.append(f"/* --- Source: {mod}.c --- */")
            out.append(clean.strip())
            out.append("")

    out.append("#ifdef __cplusplus")
    out.append("}")
    out.append("#endif")
    out.append("")
    out.append("#endif /* CPLUS_IMPLEMENTATION */")
    out.append("")

    out_file.write_text("\n".join(out), encoding="utf-8")
    print(f"[OK] Generated amalgamated header: {out_file} ({os.path.getsize(out_file):,} bytes)")

def generate_standalones():
    STANDALONE_DIR.mkdir(parents=True, exist_ok=True)
    core_text = (INCLUDE_DIR / "cp_core.h").read_text(encoding="utf-8")
    clean_core = strip_header_guards(strip_local_includes(core_text))

    for mod in MODULES:
        if mod == "cp_core":
            continue
        h_path = INCLUDE_DIR / f"{mod}.h"
        s_path = SRC_DIR / f"{mod}.c"
        if not h_path.exists():
            continue

        h_text = h_path.read_text(encoding="utf-8")
        s_text = s_path.read_text(encoding="utf-8") if s_path.exists() else ""

        sys_incs = extract_system_includes(core_text) | extract_system_includes(h_text) | extract_system_includes(s_text)
        sorted_sys = sorted(list(sys_incs))

        macro_name = f"CPLUS_{mod.upper()}_H"
        impl_name = f"CPLUS_{mod.upper()}_IMPLEMENTATION"

        out = []
        out.append(f"/**")
        out.append(f" * @file {mod}.h (Standalone Single-Header Module)")
        out.append(f" * @brief Cplus standalone module for {mod}.")
        out.append(f" *")
        out.append(f" * In ONE file define:")
        out.append(f" *   #define {impl_name}")
        out.append(f" *   #include \"{mod}.h\"")
        out.append(f" */")
        out.append(f"#ifndef {macro_name}")
        out.append(f"#define {macro_name}")
        out.append("")
        out.append("#ifndef _POSIX_C_SOURCE")
        out.append("#define _POSIX_C_SOURCE 200809L")
        out.append("#endif")
        out.append("#ifndef _GNU_SOURCE")
        out.append("#define _GNU_SOURCE")
        out.append("#endif")
        for inc in sorted_sys:
            out.append(inc)
        out.append("")
        out.append("/* Core types dependency */")
        out.append("#ifndef CPLUS_CORE_H")
        out.append("#define CPLUS_CORE_H")
        out.append(clean_core.strip())
        out.append("#endif /* CPLUS_CORE_H */")
        out.append("")
        out.append(strip_header_guards(strip_local_includes(h_text)).strip())
        out.append("")
        out.append(f"#endif /* {macro_name} */")
        out.append("")

        if s_text:
            out.append(f"#ifdef {impl_name}")
            out.append(strip_local_includes(s_text).strip())
            out.append(f"#endif /* {impl_name} */")
            out.append("")

        out_path = STANDALONE_DIR / f"{mod}.h"
        out_path.write_text("\n".join(out), encoding="utf-8")
        print(f"[OK] Generated standalone module: {out_path} ({os.path.getsize(out_path):,} bytes)")

if __name__ == "__main__":
    generate_amalgamated()
    generate_standalones()
