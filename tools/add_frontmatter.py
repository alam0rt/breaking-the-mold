#!/usr/bin/env python3
"""Add retrieval front matter (title/category/tags) to docs/*.md that lack it."""
import re
import sys
from pathlib import Path

DOCS = Path("docs")
DRY = "--apply" not in sys.argv

# generic tokens that add no retrieval value
STOP = {
    "docs", "md", "the", "and", "a", "of", "to", "for", "with", "in", "on",
    "readme", "index", "notes", "doc", "v1", "v2", "v3", "complete", "session",
}

def humanize(name: str) -> str:
    return re.sub(r"[-_]+", " ", name).strip().title()

def tokens_from(rel: Path) -> list:
    raw = []
    # directory components under docs/ (skip 'docs' itself)
    for part in rel.parent.parts:
        raw += re.split(r"[-_/]", part.lower())
    # filename stem
    raw += re.split(r"[-_]", rel.stem.lower())
    # split CamelCase / digit boundaries inside tokens
    out = []
    for t in raw:
        for piece in re.findall(r"[a-z]+|\d+", t):
            out.append(piece)
    # dedupe, drop stopwords + pure short numbers, preserve order
    seen, tags = set(), []
    for t in out:
        if t in STOP or t in seen or (t.isdigit() and len(t) < 3) or len(t) < 2:
            continue
        seen.add(t)
        tags.append(t)
    return tags[:10]

def title_of(text: str, rel: Path) -> str:
    for line in text.splitlines():
        m = re.match(r"#\s+(.*\S)", line)
        if m:
            return m.group(1).strip()
    return humanize(rel.stem)

def main():
    changed = 0
    for f in sorted(DOCS.rglob("*.md")):
        text = f.read_text()
        if text.lstrip().startswith("---"):
            continue  # already has front matter
        rel = f.relative_to(".")
        title = title_of(text, rel).replace('"', "'")
        category = str(f.parent.relative_to(DOCS)) if f.parent != DOCS else "root"
        tags = tokens_from(f.relative_to(DOCS))
        fm = (f'---\ntitle: "{title}"\ncategory: {category}\n'
              f'tags: [{", ".join(tags)}]\n---\n\n')
        if DRY:
            print(f"### {rel}\n{fm}")
        else:
            f.write_text(fm + text)
        changed += 1
    print(f"\n{'WOULD ADD' if DRY else 'ADDED'} front matter to {changed} files", file=sys.stderr)

if __name__ == "__main__":
    main()
