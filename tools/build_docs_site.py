#!/usr/bin/env python3
"""
tools/build_docs_site.py
Generates the rich Black & Azure HTML manual pages and bundles for C+:
1. Converts all 18 docs/man/*.md files to rich HTML with C syntax highlighting.
2. Generates docs/data/manuals.js for instant client-side rendering inside index.html.
3. Generates standalone docs/man/cp_*.html pages with matching Black & Azure theme.
4. Updates tools/build_docs_data.py to use .html links instead of raw .md.
"""

import os
import re
import html
import json
import markdown
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MAN_DIR = ROOT / "docs" / "man"
DATA_DIR = ROOT / "docs" / "data"

MODULES = [
    ("cp_core", "Core Architecture", "RAII, arenas, panics, macros, types", "core"),
    ("cp_mem", "Memory Subsystem", "Linear arenas, scratch rollbacks, pool slabs", "mem"),
    ("cp_str", "Strings & Builders", "Immutable strings, dynamic builders, views", "str"),
    ("cp_fs", "Filesystem & IO", "Atomic writes, memory-mapped files, path ops", "fs"),
    ("cp_ds", "Data Structures", "Type-safe vectors, deques, dynamic arrays", "ds"),
    ("cp_graph", "Graphs & Paths", "Adjacency lists, Dijkstra, BFS/DFS, topologies", "graph"),
    ("cp_algo", "Algorithms & Sorting", "QuickSort, binary search, partition, reverse", "algo"),
    ("cp_math", "SIMD & Fast Math", "Vectors, quaternions, fast inverse sqrt, clamps", "math"),
    ("cp_thread", "Concurrency & Workers", "Thread pool, work stealing, atomics, mutexes", "thread"),
    ("cp_net", "High-Performance Net", "TCP stream server/client, non-blocking sockets", "net"),
    ("cp_tui", "Terminal UI & Telemetry", "75k FPS terminal rendering, frames, widgets", "tui"),
    ("cp_hash", "Cryptographic Hashes", "Murmur3, FNV-1a, Blake3, SipHash, CRC32", "hash"),
    ("cp_map", "Hash Maps & Sets", "Open-addressing robin-hood hash table", "map"),
    ("cp_args", "CLI Argument Parser", "Positional args, flags, options, help generator", "args"),
    ("cp_time", "Precision Clocks", "Nanosecond timers, stopwatches, UTC time", "time"),
    ("cp_json", "Streaming JSON Engine", "Zero-alloc JSON parser, serializer, DOM", "json"),
    ("cp_stack", "Cache-Friendly Stack", "Linear cache-line contiguous stack allocator", "stack"),
    ("cp_os", "Platform Subsystem", "Process PID, memory usage, CPU cores, env vars", "os")
]

KEYWORDS = set([
    'typedef', 'struct', 'union', 'enum', 'return', 'void', 'const', 'static', 'inline',
    'extern', 'volatile', 'register', 'auto', 'sizeof', 'if', 'else', 'for', 'while',
    'do', 'switch', 'case', 'break', 'continue', 'default', 'goto'
])

TYPES = set([
    'u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64', 'usize', 'isize',
    'f32', 'f64', 'bool', 'true', 'false', 'NULL', 'uintptr_t', 'intptr_t',
    'size_t', 'ssize_t', 'int', 'char', 'float', 'double', 'unsigned', 'signed', 'short', 'long',
    'Arena', 'TempArena', 'Pool', 'Vec', 'HashMap', 'Map', 'String', 'Stack', 'RingBuffer', 'RingBuf',
    'Graph', 'Socket', 'Mutex', 'Cond', 'ThreadPool', 'Thread', 'Stopwatch', 'JsonValue', 'JsonDoc',
    'cp_arena_t', 'cp_arena_chunk_t', 'cp_temp_arena_t', 'cp_pool_t', 'cp_str_t', 'cp_vec_header_t',
    'cp_hashmap_t', 'cp_stack_t', 'cp_ringbuf_t', 'cp_graph_t', 'cp_socket_t', 'cp_mutex_t',
    'cp_cond_t', 'cp_thread_pool_t', 'cp_stopwatch_t', 'cp_json_val_t'
])

def highlight_c_line(line):
    tokens = re.split(r'("[^"]*"|\'[^\']*\'|\b\w+\b|[^\w\s"\']+)', line)
    out = []
    for t in tokens:
        if not t:
            continue
        if t.startswith('"') or t.startswith("'"):
            out.append(f'<span class="tok-string">{html.escape(t)}</span>')
        elif t in KEYWORDS:
            out.append(f'<span class="tok-keyword">{t}</span>')
        elif t in TYPES or t.endswith('_t'):
            out.append(f'<span class="tok-type">{t}</span>')
        elif re.match(r'^(0x[0-9a-fA-F]+|\d+(?:\.\d+)?f?)$', t):
            out.append(f'<span class="tok-number">{t}</span>')
        elif re.match(r'^[A-Z][a-zA-Z0-9_]*$', t):
            out.append(f'<span class="tok-func">{html.escape(t)}</span>')
        else:
            out.append(html.escape(t))
    return ''.join(out)

def highlight_c_code(raw_code):
    lines = raw_code.split('\n')
    out_lines = []
    in_multi_comment = False

    for line in lines:
        stripped = line.strip()
        if in_multi_comment:
            if '*/' in line:
                idx = line.find('*/') + 2
                comm = line[:idx]
                rest = line[idx:]
                in_multi_comment = False
                out_lines.append(f'<span class="tok-comment">{html.escape(comm)}</span>' + highlight_c_line(rest))
            else:
                out_lines.append(f'<span class="tok-comment">{html.escape(line)}</span>')
            continue
            
        if '/*' in line:
            if '*/' in line:
                p1 = line.find('/*')
                p2 = line.find('*/') + 2
                out_lines.append(
                    highlight_c_line(line[:p1]) +
                    f'<span class="tok-comment">{html.escape(line[p1:p2])}</span>' +
                    highlight_c_line(line[p2:])
                )
                continue
            else:
                p1 = line.find('/*')
                in_multi_comment = True
                out_lines.append(
                    highlight_c_line(line[:p1]) +
                    f'<span class="tok-comment">{html.escape(line[p1:])}</span>'
                )
                continue

        if stripped.startswith('#'):
            h = html.escape(line)
            h = re.sub(r'^(#\w+)', r'<span class="tok-macro">\1</span>', h)
            h = re.sub(r'(&quot;.*?&quot;|&lt;.*?&gt;)', r'<span class="tok-string">\1</span>', h)
            out_lines.append(h)
            continue
            
        if '//' in line:
            parts = line.split('//', 1)
            out_lines.append(highlight_c_line(parts[0]) + f'<span class="tok-comment">{html.escape("//" + parts[1])}</span>')
            continue
            
        out_lines.append(highlight_c_line(line))
        
    return '\n'.join(out_lines)

def convert_markdown(md_text):
    code_blocks = []
    
    def code_repl(m):
        lang = (m.group(1) or 'c').lower()
        raw = m.group(2).strip()
        idx = len(code_blocks)
        
        if lang in ('c', 'h'):
            hl = highlight_c_code(raw)
        else:
            hl = html.escape(raw)
            
        # Code block container with header and copy button
        block_html = (
            f'<div class="code-container">'
            f'<div class="code-header">'
            f'<span class="code-lang">{html.escape(lang.upper())}</span>'
            f'<button class="copy-btn" onclick="copyCode(this)">Copy</button>'
            f'</div>'
            f'<pre class="code-pre"><code class="language-{lang}">{hl}</code></pre>'
            f'</div>'
        )
        code_blocks.append(block_html)
        return f"\n\n@@@CODE_BLOCK_{idx}@@@\n\n"
        
    # Replace fenced code blocks
    processed_md = re.sub(r'```(\w+)?\n(.*?)```', code_repl, md_text, flags=re.DOTALL)
    
    # Run python markdown parser
    raw_html = markdown.markdown(processed_md, extensions=['tables'])
    
    # Post-process headings with slugified anchor IDs and secondary anchor spans
    def heading_repl(m):
        level = m.group(1)
        text = m.group(2)
        clean = re.sub(r'<[^>]+>', '', text)
        slug = re.sub(r'[^a-zA-Z0-9]+', '-', clean).strip('-').lower()
        anchors = [slug]
        for word in re.findall(r'[a-zA-Z0-9_]+', clean):
            w_lower = word.lower()
            if len(w_lower) >= 3 and w_lower not in anchors:
                anchors.append(w_lower)
        spans = ''.join(f'<span id="{a}" class="anchor-target"></span>' for a in anchors if a != slug)
        return f'{spans}<h{level} id="{slug}"><a href="#{slug}" class="heading-anchor">#</a> {text}</h{level}>'
        
    raw_html = re.sub(r'<h([1-6])>(.*?)</h\1>', heading_repl, raw_html)
    
    # Wrap tables
    raw_html = re.sub(r'(<table>.*?</table>)', r'<div class="table-wrapper">\1</div>', raw_html, flags=re.DOTALL)

    # Add anchor spans to table cells with code
    def table_code_repl(m):
        full_cell = m.group(0)
        code_body = m.group(1)
        clean_name = code_body.split('(')[0].split('.')[-1].strip()
        slug = re.sub(r'[^a-zA-Z0-9_]+', '', clean_name).lower()
        if len(slug) >= 2:
            return f'<td><span id="{slug}" class="anchor-target"></span><code>{code_body}</code></td>'
        return full_cell
        
    raw_html = re.sub(r'<td><code>([^<]+)</code></td>', table_code_repl, raw_html)
    
    # Restore code blocks
    for i, blk in enumerate(code_blocks):
        raw_html = raw_html.replace(f"<p>@@@CODE_BLOCK_{i}@@@</p>", blk)
        raw_html = raw_html.replace(f"@@@CODE_BLOCK_{i}@@@", blk)
        
    return raw_html

def get_shared_css():
    return """
    :root {
      --bg: #000000;
      --bg-dark: #050505;
      --surface: #0a0a0a;
      --surface-hover: #121214;
      --border: #1a1a1a;
      --border-bright: #27272a;
      --azure: #16e1e5;
      --azure-dim: rgba(22, 225, 229, 0.08);
      --text: #c4c4c8;
      --text-bright: #ffffff;
      --text-dim: #71717a;
      
      /* VS Code / Neovim 1:1 C Syntax Highlighting */
      --tok-keyword: #c586c0;
      --tok-type: #4ec9b0;
      --tok-func: #dcdcaa;
      --tok-string: #ce9178;
      --tok-number: #b5cea8;
      --tok-comment: #6a9955;
      --tok-macro: #c586c0;
      
      --font-sans: -apple-system, BlinkMacSystemFont, 'Inter', 'Segoe UI', Roboto, sans-serif;
      --font-mono: 'JetBrains Mono', 'Fira Code', 'Cascadia Code', monospace;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      background: var(--bg);
      color: var(--text);
      font-family: var(--font-sans);
      font-size: 14px;
      line-height: 1.65;
    }

    /* Code Highlight Styles */
    .tok-keyword { color: var(--tok-keyword); font-weight: 600; }
    .tok-type { color: var(--tok-type); font-weight: 500; }
    .tok-func { color: var(--tok-func); }
    .tok-string { color: var(--tok-string); }
    .tok-number { color: var(--tok-number); }
    .tok-comment { color: var(--tok-comment); font-style: italic; }
    .tok-macro { color: var(--tok-macro); font-weight: 600; }

    /* Code Container */
    .code-container {
      background: #050505;
      border: 1px solid var(--border);
      border-radius: 6px;
      margin: 16px 0;
      overflow: hidden;
    }
    .code-header {
      background: #0a0a0a;
      border-bottom: 1px solid var(--border);
      padding: 6px 12px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .code-lang {
      font-family: var(--font-mono);
      font-size: 11px;
      font-weight: 500;
      color: var(--text-dim);
      letter-spacing: 0.5px;
    }
    .copy-btn {
      background: transparent;
      border: 1px solid var(--border);
      color: var(--text-dim);
      padding: 2px 8px;
      border-radius: 4px;
      font-size: 11px;
      font-family: var(--font-mono);
      cursor: pointer;
      transition: all 0.15s ease;
    }
    .copy-btn:hover {
      background: var(--azure-dim);
      border-color: var(--azure);
      color: var(--azure);
    }
    .code-pre {
      padding: 14px;
      overflow-x: auto;
      font-family: var(--font-mono);
      font-size: 12.5px;
      line-height: 1.5;
      color: var(--text-bright);
    }

    /* Headings */
    h1 {
      color: var(--text-bright);
      font-size: 24px;
      font-weight: 700;
      margin-bottom: 14px;
      padding-bottom: 10px;
      border-bottom: 1px solid var(--border);
      letter-spacing: -0.4px;
    }
    h2 {
      color: var(--text-bright);
      font-size: 18px;
      font-weight: 600;
      margin-top: 32px;
      margin-bottom: 12px;
      letter-spacing: -0.2px;
    }
    h3 {
      color: #f4f4f5;
      font-size: 15px;
      font-weight: 600;
      margin-top: 22px;
      margin-bottom: 8px;
    }
    p { margin-bottom: 14px; }
    strong { color: var(--text-bright); }
    hr {
      border: 0;
      border-top: 1px solid var(--border);
      margin: 28px 0;
    }

    /* Inline Code */
    p code, li code, td code {
      background: #0a0a0a;
      color: #f4f4f5;
      font-family: var(--font-mono);
      font-size: 12px;
      padding: 2px 5px;
      border-radius: 3px;
      border: 1px solid var(--border);
    }

    /* Lists */
    ul, ol {
      margin-left: 20px;
      margin-bottom: 16px;
    }
    li { margin-bottom: 5px; }

    /* Tables */
    .table-wrapper {
      overflow-x: auto;
      margin: 18px 0;
      border: 1px solid var(--border);
      border-radius: 6px;
    }
    table {
      width: 100%;
      border-collapse: collapse;
      font-size: 13px;
      background: #050505;
    }
    th {
      background: #0a0a0a;
      color: var(--text-dim);
      text-align: left;
      padding: 8px 12px;
      border-bottom: 1px solid var(--border);
      font-weight: 600;
      font-family: var(--font-mono);
      font-size: 11px;
      letter-spacing: 0.5px;
      text-transform: uppercase;
    }
    td {
      padding: 8px 12px;
      border-bottom: 1px solid #141414;
      color: var(--text);
    }
    tr:last-child td { border-bottom: none; }
    tr:nth-child(even) td { background: rgba(255, 255, 255, 0.01); }

    /* Heading Anchor */
    .heading-anchor {
      color: var(--text-dim);
      opacity: 0.3;
      text-decoration: none;
      margin-right: 6px;
      transition: opacity 0.15s, color 0.15s;
    }
    .heading-anchor:hover { opacity: 1; color: var(--azure); }

    /* Anchors & Highlighting */
    .anchor-target {
      display: block;
      position: relative;
      top: -70px;
      visibility: hidden;
    }
    .highlight-target {
      animation: targetPulse 2s ease-out;
    }
    @keyframes targetPulse {
      0% { background: rgba(22, 225, 229, 0.2) !important; outline: 1px solid var(--azure); border-radius: 4px; }
      100% { background: transparent; }
    }
    """

def generate_standalone_html(mod_key, title, body_html):
    css = get_shared_css()
    
    # Generate sidebar links
    sidebar_items = []
    for k, name, desc, cat in MODULES:
        active = 'active' if k == mod_key else ''
        sidebar_items.append(
            f'<a href="{k}.html" class="nav-item {active}">'
            f'<span class="nav-title">{k}</span>'
            f'<span class="nav-sub">{name}</span>'
            f'</a>'
        )
    sidebar_html = "\n".join(sidebar_items)

    page_html = f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>C+ Manual // {title} ({mod_key})</title>
  <style>
    {css}

    /* Page Layout */
    .app-container {{
      display: flex;
      flex-direction: column;
      min-height: 100vh;
      background: #000000;
    }}
    .site-header {{
      background: #000000;
      border-bottom: 1px solid var(--border);
      padding: 8px 24px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      position: sticky;
      top: 0;
      z-index: 50;
    }}
    .brand-link {{
      display: flex;
      align-items: center;
      gap: 10px;
      text-decoration: none;
    }}
    .brand-badge {{
      background: #000000;
      border: 1px solid var(--azure);
      color: #ffffff;
      padding: 2px 8px;
      border-radius: 4px;
      font-family: var(--font-mono);
      font-weight: 700;
      font-size: 13px;
    }}
    .brand-badge span {{ color: var(--azure); }}
    .brand-text {{
      color: var(--text-bright);
      font-weight: 600;
      font-size: 13px;
    }}
    .brand-text span {{
      color: var(--text-dim);
      font-weight: 400;
      font-size: 11px;
      margin-left: 6px;
    }}
    .header-nav {{
      display: flex;
      gap: 10px;
      align-items: center;
    }}
    .header-nav a {{
      color: var(--text-dim);
      text-decoration: none;
      font-size: 11px;
      padding: 4px 10px;
      border-radius: 4px;
      border: 1px solid var(--border);
      transition: all 0.15s;
    }}
    .header-nav a:hover {{
      color: var(--azure);
      border-color: var(--azure);
    }}

    .main-body {{
      display: flex;
      flex: 1;
    }}
    .sidebar {{
      width: 250px;
      background: #000000;
      border-right: 1px solid var(--border);
      padding: 12px 8px;
      flex-shrink: 0;
      overflow-y: auto;
      height: calc(100vh - 45px);
      position: sticky;
      top: 45px;
    }}
    .sidebar-heading {{
      font-family: var(--font-mono);
      font-size: 10px;
      font-weight: 600;
      color: var(--text-dim);
      text-transform: uppercase;
      letter-spacing: 1px;
      padding: 6px 10px;
      margin-bottom: 4px;
    }}
    .nav-item {{
      display: flex;
      flex-direction: column;
      padding: 6px 10px;
      border-radius: 4px;
      text-decoration: none;
      margin-bottom: 2px;
      border-left: 2px solid transparent;
      transition: all 0.1s ease;
    }}
    .nav-item:hover {{
      background: var(--surface-hover);
    }}
    .nav-item.active {{
      background: rgba(22, 225, 229, 0.04);
      border-left-color: var(--azure);
    }}
    .nav-title {{
      font-family: var(--font-mono);
      font-size: 12px;
      font-weight: 500;
      color: var(--text-dim);
    }}
    .nav-item.active .nav-title {{
      color: #ffffff;
      font-weight: 600;
    }}
    .nav-sub {{
      font-size: 10px;
      color: #52525b;
    }}

    .content-area {{
      flex: 1;
      padding: 32px 48px;
      max-width: 900px;
      margin: 0 auto;
    }}
  </style>
</head>
<body>
  <div class="app-container">
    <header class="site-header">
      <a href="../index.html" class="brand-link">
        <div class="brand-badge">C<span>+</span></div>
        <div class="brand-text">Foundation Library <span>// Formal Manual</span></div>
      </a>
      <div class="header-nav">
        <a href="../index.html">&larr; Telescope Search (/)</a>
        <a href="https://github.com/IovAnto/Cplus-Lib" target="_blank">GitHub Repo</a>
      </div>
    </header>

    <div class="main-body">
      <aside class="sidebar">
        <div class="sidebar-heading">18 Core Subsystems</div>
        {sidebar_html}
      </aside>

      <main class="content-area">
        {body_html}
      </main>
    </div>
  </div>

  <script>
    function copyCode(btn) {{
      const container = btn.closest('.code-container');
      const code = container.querySelector('code').innerText;
      navigator.clipboard.writeText(code).then(() => {{
        const orig = btn.innerText;
        btn.innerText = 'Copied!';
        btn.style.borderColor = 'var(--azure)';
        setTimeout(() => {{
          btn.innerText = orig;
          btn.style.borderColor = '';
        }}, 1500);
      }});
    }}

    function scrollToAnchor() {{
      const rawHash = window.location.hash.slice(1);
      if (!rawHash) return;
      const cleanHash = decodeURIComponent(rawHash).toLowerCase().replace(/[^a-z0-9_]/g, '');
      let target = document.getElementById(rawHash) || document.getElementById(cleanHash);
      if (!target) {{
        const candidates = document.querySelectorAll('h1, h2, h3, h4, td, code, th, span[id]');
        for (const el of candidates) {{
          const id = (el.id || '').toLowerCase();
          const text = (el.textContent || '').toLowerCase();
          if (id === cleanHash || id.includes(cleanHash) || text.includes(cleanHash)) {{
            target = el.closest('tr') || el;
            break;
          }}
        }}
      }}
      if (target) {{
        target.scrollIntoView({{ behavior: 'smooth', block: 'center' }});
        target.classList.add('highlight-target');
        setTimeout(() => {{ target.classList.remove('highlight-target'); }}, 2500);
      }}
    }}
    window.addEventListener('DOMContentLoaded', scrollToAnchor);
    window.addEventListener('hashchange', scrollToAnchor);
  </script>
</body>
</html>"""
    return page_html

def main():
    print("Building Rich C+ Documentation Site...")
    manuals_dict = {}

    for mod_key, mod_title, mod_desc, mod_cat in MODULES:
        md_file = MAN_DIR / f"{mod_key}.md"
        if not md_file.exists():
            print(f"Warning: {md_file} not found")
            continue
            
        md_content = md_file.read_text(encoding="utf-8")
        body_html = convert_markdown(md_content)
        manuals_dict[mod_key] = body_html
        
        # Write standalone HTML file
        standalone_html = generate_standalone_html(mod_key, mod_title, body_html)
        out_html_path = MAN_DIR / f"{mod_key}.html"
        out_html_path.write_text(standalone_html, encoding="utf-8")
        print(f"  [OK] Generated standalone manual: {out_html_path.name}")

    # Generate docs/data/manuals.js
    js_content = "// Generated by tools/build_docs_site.py - C+ Rendered Manuals\n"
    js_content += "window.CPLUS_MANUALS = " + json.dumps(manuals_dict, indent=2) + ";\n"
    manuals_js_path = DATA_DIR / "manuals.js"
    manuals_js_path.write_text(js_content, encoding="utf-8")
    print(f"[OK] Generated {len(manuals_dict)} bundled manuals in {manuals_js_path.name}")

if __name__ == "__main__":
    main()
