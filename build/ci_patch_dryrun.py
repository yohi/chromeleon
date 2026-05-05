#!/usr/bin/env python3
"""CI 環境でパッチファイルの形式を検証する。

Chromium ソースツリーが無い CI 環境では、パッチファイルの
diff 構文が有効であることのみを検証する。
"""

import subprocess
import sys
from pathlib import Path


def main() -> None:
    project_root = Path(__file__).resolve().parent.parent
    series_file = project_root / "patches" / "series"

    if not series_file.exists():
        print("patches/series not found, skipping.")
        return

    patches = [
        line.strip()
        for line in series_file.read_text().splitlines()
        if line.strip() and not line.strip().startswith("#")
    ]

    errors = 0
    for patch_name in patches:
        patch_path = project_root / "patches" / patch_name
        if not patch_path.exists():
            print(f"ERROR: {patch_name} listed in series but file not found")
            errors += 1
            continue

        result = subprocess.run(
            ["git", "apply", "--stat", str(patch_path)],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            print(f"ERROR: {patch_name} has invalid diff format")
            errors += 1
        else:
            print(f"  OK: {patch_name}")

    if errors:
        print(f"{errors} patch(es) failed validation", file=sys.stderr)
        sys.exit(1)

    print(f"All {len(patches)} patches validated.")


if __name__ == "__main__":
    main()
