#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN = "".join(("208", "234", "161"))
REQUIRED_EXTENSIONS = {".c", ".h"}


def main() -> None:
    source_files = [path for path in (ROOT / "src").rglob("*") if path.suffix in REQUIRED_EXTENSIONS]
    if not source_files:
        raise SystemExit("no C source/header files found")
    for path in ROOT.rglob("*"):
        if ".git" in path.parts or path.is_dir():
            continue
        if path.suffix in {".c", ".h", ".md", ".py", ".txt", ".kernel"} or path.name == "Makefile":
            text = path.read_text(encoding="utf-8", errors="ignore")
            if FORBIDDEN in text:
                raise SystemExit(f"forbidden student identifier remains in {path.relative_to(ROOT)}")
            if "".join(("/", "Users", "/")) in text:
                raise SystemExit(f"local home path remains in {path.relative_to(ROOT)}")
    print(f"validated {len(source_files)} C source/header file(s)")


if __name__ == "__main__":
    main()
