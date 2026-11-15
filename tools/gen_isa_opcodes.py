#!/usr/bin/env python3
"""Generate include/haeun/isa_opcodes.h from tools/isa/haeun_isa.json."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JSON_PATH = ROOT / "tools" / "isa" / "haeun_isa.json"
HEADER_PATH = ROOT / "include" / "haeun" / "isa_opcodes.h"


def render_header(data: dict) -> str:
    """Build the C header text from ISA metadata."""
    lines = [
        "#ifndef HAEUN_ISA_OPCODES_H",
        "#define HAEUN_ISA_OPCODES_H",
        "",
        "/* Generated from tools/isa/haeun_isa.json — do not edit by hand. */",
        "/* Run: python tools/gen_isa_opcodes.py */",
        "",
        "enum {",
    ]
    for entry in data["opcodes"]:
        lines.append(f"    HAEUN_OP_{entry['id']} = {entry['op']},")
    lines.extend(
        [
            "};",
            "",
            "#endif",
            "",
        ]
    )
    return "\n".join(lines)


def main(argv=None) -> int:
    """Write or check the generated opcode header."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="exit 1 if the header does not match the JSON",
    )
    args = parser.parse_args(argv)
    data = json.loads(JSON_PATH.read_text(encoding="utf-8"))
    text = render_header(data)
    if args.check:
        existing = HEADER_PATH.read_text(encoding="utf-8") if HEADER_PATH.exists() else ""
        if existing != text:
            print("haeun: isa_opcodes.h is out of date; run tools/gen_isa_opcodes.py", file=sys.stderr)
            return 1
        return 0
    HEADER_PATH.parent.mkdir(parents=True, exist_ok=True)
    HEADER_PATH.write_text(text, encoding="utf-8", newline="\n")
    print(f"wrote {HEADER_PATH.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
