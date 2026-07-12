#!/usr/bin/env python3
# Helper for the Russian->English comment translation pass (#56).
# Replaces specific 1-based line numbers in a file with provided English text,
# preserving the file's original line ending. Usage:
#   python xlate_comments.py <file> <replacements.json>
# replacements.json = { "<lineno>": "<full new line text without newline>", ... }
import sys, json, io

def main():
    path, repl_path = sys.argv[1], sys.argv[2]
    with open(repl_path, "r", encoding="utf-8") as f:
        repl = json.load(f)
    repl = {int(k): v for k, v in repl.items()}

    with open(path, "rb") as f:
        raw = f.read()
    nl = b"\r\n" if b"\r\n" in raw else b"\n"
    # split keeping no trailing empty handling issues
    text = raw.decode("utf-8", errors="surrogateescape")
    # normalize to \n for splitting, remember we re-join with nl
    lines = text.split("\r\n") if nl == b"\r\n" else text.split("\n")

    for ln, new in repl.items():
        idx = ln - 1
        if idx < 0 or idx >= len(lines):
            print(f"WARN: line {ln} out of range ({len(lines)} lines)")
            continue
        lines[idx] = new

    out = (nl.decode("ascii")).join(lines)
    with open(path, "w", encoding="utf-8", errors="surrogateescape", newline="") as f:
        f.write(out)
    print(f"OK: {path} ({len(repl)} lines)")

if __name__ == "__main__":
    main()
