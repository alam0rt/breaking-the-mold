"""Extract CamelCase tokens from symbol_addrs.txt for compound attacks."""
import re
import sys
from pathlib import Path

with open("symbol_addrs.txt") as f:
    text = f.read()

# Pull every identifier that's at least 4 chars and contains a lower-case letter
ids = set(re.findall(r"[A-Za-z][A-Za-z0-9_]{3,40}", text))

# Split CamelCase / underscores
tokens: set[str] = set()
for ident in ids:
    # Split on underscore
    for part in ident.split("_"):
        # Split CamelCase: lowercase-uppercase boundaries
        # e.g. AddPhartHeads → Add, Phart, Heads
        chunks = re.findall(r"[A-Z][a-z0-9]+|[A-Z]+(?=[A-Z]|$)|[a-z0-9]+", part)
        for c in chunks:
            cu = c.upper()
            if 3 <= len(cu) <= 18 and not cu.isdigit():
                tokens.add(cu)

# Filter: drop common boring tokens (PSX/PSY-Q/runtime stuff)
DROP = {
    "INIT", "DEINIT", "FUNC", "FUN", "DAT", "ASM", "STR", "PROG",
    "GFX", "GET", "SET", "ADD", "REM", "REMOVE", "DELETE", "DEL",
    "NEW", "FREE", "ALLOC", "MAKE", "CREATE", "DRAW", "RENDER",
    "FROM", "INTO", "WITH", "AND", "FOR", "THE", "USE",
    "TICK", "UPDATE", "LOAD", "READ", "WRITE", "SAVE", "OPEN",
    "CLOSE", "FILE", "DATA", "INFO", "BIT", "BYTE", "WORD", "DWORD",
    "INT", "UNSIGNED", "SIGNED", "FLOAT", "DOUBLE", "BOOL",
    "CHAR", "STRING", "POINTER", "ARRAY",
    "PSX", "PSYQ", "GTE", "BIOS", "MEM", "RAM", "VRAM", "CD", "DMA",
    "CLUT", "OT", "PRIM", "POLY", "SPR", "FT", "F3", "F4", "G3", "G4",
    "TPAGE", "PAGE", "LIST", "QUEUE", "BUFFER", "STACK", "HEAP",
    "RET", "ARG", "PARAM", "VAR", "TMP", "STK",
    "IDX", "INDEX", "COUNT", "NUM", "AMT", "AMOUNT", "SIZE", "LEN",
    "POS", "POSITION", "COORD", "DELTA", "OFFSET", "BASE",
}
tokens = {t for t in tokens if t not in DROP}
tokens = sorted(tokens)

with open("/tmp/sym_tokens.txt", "w") as f:
    for t in tokens:
        f.write(t + "\n")
print(f"wrote {len(tokens)} CamelCase tokens to /tmp/sym_tokens.txt")
print("Sample:", ", ".join(tokens[:30]))
print("Game-specific examples:")
for t in tokens:
    if any(k in t for k in ("PHART", "WILL", "SWIRL", "ENEMA", "PHOENI", "ORB",
                             "SHRIN", "JOE", "KLAY", "KLOG", "FINN", "MEGA",
                             "MENU", "BOSS", "HEAD", "GLEN", "BIRD", "CLAY")):
        print(f"  {t}")
