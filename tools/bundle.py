#!/usr/bin/env python3
"""
tools/bundle.py
On-demand custom single-header builder for Cplus.
Usage:
    python3 tools/bundle.py --modules mem,stack,tui -o my_cplus.h
"""

import sys
import argparse
from pathlib import Path
from amalgamate import (
    ROOT_DIR, INCLUDE_DIR, SRC_DIR, MODULES,
    extract_system_includes, strip_local_includes, strip_header_guards
)

def build_custom_bundle(selected_modules, output_path):
    # Always include cp_core as base
    mods = ["cp_core"]
    for m in selected_modules:
        m_name = m.strip()
        if not m_name.startswith("cp_"):
            m_name = f"cp_{m_name}"
        if m_name in MODULES and m_name not in mods:
            mods.append(m_name)

    all_sys = set()
    h_content = {}
    s_content = {}

    for mod in mods:
        hp = INCLUDE_DIR / f"{mod}.h"
        if hp.exists():
            txt = hp.read_text(encoding="utf-8")
            all_sys.update(extract_system_includes(txt))
            h_content[mod] = txt
        sp = SRC_DIR / f"{mod}.c"
        if sp.exists():
            txt = sp.read_text(encoding="utf-8")
            all_sys.update(extract_system_includes(txt))
            s_content[mod] = txt

    out = []
    out.append("/**")
    out.append(f" * Custom Cplus Single-Header Bundle")
    out.append(f" * Modules: {', '.join(mods)}")
    out.append(" */")
    out.append("#ifndef CPLUS_CUSTOM_BUNDLE_H")
    out.append("#define CPLUS_CUSTOM_BUNDLE_H")
    out.append("")
    out.append("#ifndef _POSIX_C_SOURCE")
    out.append("#define _POSIX_C_SOURCE 200809L")
    out.append("#endif")
    out.append("#ifndef _GNU_SOURCE")
    out.append("#define _GNU_SOURCE")
    out.append("#endif")
    for inc in sorted(list(all_sys)):
        out.append(inc)
    out.append("")
    out.append("#ifdef __cplusplus")
    out.append('extern "C" {')
    out.append("#endif")
    out.append("")

    for mod in mods:
        if mod in h_content:
            clean = strip_local_includes(h_content[mod])
            clean = strip_header_guards(clean)
            out.append(f"/* Module: {mod} */")
            out.append(clean.strip())
            out.append("")

    out.append("#ifdef __cplusplus")
    out.append("}")
    out.append("#endif")
    out.append("")
    out.append("#endif /* CPLUS_CUSTOM_BUNDLE_H */")
    out.append("")
    out.append("#ifdef CPLUS_IMPLEMENTATION")
    out.append("#ifdef __cplusplus")
    out.append('extern "C" {')
    out.append("#endif")
    out.append("")

    for mod in mods:
        if mod in s_content:
            clean = strip_local_includes(s_content[mod])
            out.append(f"/* Implementation: {mod}.c */")
            out.append(clean.strip())
            out.append("")

    out.append("#ifdef __cplusplus")
    out.append("}")
    out.append("#endif")
    out.append("#endif /* CPLUS_IMPLEMENTATION */")
    out.append("")

    Path(output_path).write_text("\n".join(out), encoding="utf-8")
    print(f"[OK] Custom bundle generated at {output_path} with modules: {', '.join(mods)}")

def main():
    parser = argparse.ArgumentParser(description="Custom Cplus single-header builder")
    parser.add_argument("--modules", required=True, help="Comma-separated module names (e.g. mem,stack,tui)")
    parser.add_argument("-o", "--output", default="cplus_custom.h", help="Output header path")
    args = parser.parse_args()

    selected = args.modules.split(",")
    build_custom_bundle(selected, args.output)

if __name__ == "__main__":
    main()
