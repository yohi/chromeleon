#!/usr/bin/env python3
"""patches/series に従って Chromium src からパッチを逆順で除去する。"""

import os
import subprocess
import sys
from pathlib import Path


def main() -> None:
    project_root = Path(__file__).resolve().parent.parent
    series_file = project_root / "patches" / "series"

    if not series_file.exists():
        print("patches/series not found, nothing to unapply.")
        return

    chromium_src = os.environ["CHROMIUM_SRC"]
    patches = [
        line.strip()
        for line in series_file.read_text().splitlines()
        if line.strip() and not line.strip().startswith("#")
    ]

    for patch_name in reversed(patches):
        patch_path = project_root / "patches" / patch_name
        print(f"Unapplying: {patch_name}")
        result = subprocess.run(
            ["git", "apply", "--reverse", str(patch_path)],
            cwd=chromium_src,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            print(f"FAILED to unapply: {patch_name}", file=sys.stderr)
            print(result.stderr, file=sys.stderr)
            sys.exit(1)

    print(f"All {len(patches)} patches unapplied successfully.")


if __name__ == "__main__":
    main()
