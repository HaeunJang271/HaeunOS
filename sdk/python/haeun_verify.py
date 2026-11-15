"""Application HXF validators for the Developer SDK."""
from __future__ import annotations

USER_VA = 0x00108000
MAX_CODE = 4096


def _u32(b: bytes, off: int) -> int:
    return int.from_bytes(b[off : off + 4], "little")


def verify_app_hxf(data: bytes) -> tuple[bool, str]:
    """
    Check HXF1 shape required by APP_SPAWN / extract_user.

    Returns (ok, message).
    """
    if len(data) < 96:
        return False, "truncated HXF"
    if data[0:4] != b"HXF1":
        return False, "bad magic"
    isa = _u32(data, 8)
    if isa < 1 or isa > 4:
        return False, f"unsupported ISA {isa}"
    entry = _u32(data, 16)
    if entry != USER_VA:
        return False, f"entry must be 0x{USER_VA:08x} (got 0x{entry:08x})"
    table = _u32(data, 20)
    count = data[24] | (data[25] << 8)
    if count == 0 or table + count * 32 > len(data):
        return False, "bad segment table"
    found = False
    for i in range(count):
        raw = data[table + i * 32 :]
        flags = _u32(raw, 4)
        off = _u32(raw, 8)
        va = _u32(raw, 12)
        fsz = _u32(raw, 16)
        msz = _u32(raw, 20)
        if va != USER_VA or (flags & 4) == 0:
            continue
        if fsz > MAX_CODE or fsz > msz or msz > MAX_CODE:
            return False, f"segment too large ({msz})"
        if off + fsz > len(data):
            return False, "segment exceeds file"
        found = True
        break
    if not found:
        return False, "missing executable segment at user VA"
    return True, f"ISA{isa} entry=0x{entry:08x} size={len(data)}"
