#!/usr/bin/env python3
"""patches/series に従って Chromium src にパッチを適用する。"""

import os
import subprocess
import sys
from pathlib import Path


def main() -> None:
    project_root = Path(__file__).resolve().parent.parent
    series_file = project_root / "patches" / "series"

    if not series_file.exists():
        print("patches/series not found, skipping patch application.")
        return

    chromium_src = os.environ["CHROMIUM_SRC"]
    patches = [
        line.strip()
        for line in series_file.read_text().splitlines()
        if line.strip() and not line.strip().startswith("#")
    ]

    for patch_name in patches:
        patch_path = project_root / "patches" / patch_name
        print(f"Applying: {patch_name}")
        result = subprocess.run(
            ["git", "apply", "--3way", str(patch_path)],
            cwd=chromium_src,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            print(f"FAILED: {patch_name}", file=sys.stderr)
            print(result.stderr, file=sys.stderr)
            sys.exit(1)
        print(f"  OK: {patch_name}")

    print(f"All {len(patches)} patches applied successfully.")


if __name__ == "__main__":
    main()
