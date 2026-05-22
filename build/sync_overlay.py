#!/usr/bin/env python3
"""Overlay ディレクトリを Chromium src に symlink で注入する。

GN は //src/ 外のパスを参照できないため、chromium_src/overlay/ を
$CHROMIUM_SRC/chromium_src/overlay/ に symlink で載せる。
編集は自リポジトリ側 1 箇所で完結する。
"""

import os
import shutil
from pathlib import Path


def main() -> None:
    overlay_root = Path(__file__).resolve().parent.parent / "chromium_src" / "overlay"
    chromium_src = Path(os.environ["CHROMIUM_SRC"])
    target = chromium_src / "chromium_src" / "overlay"

    if target.is_symlink():
        target.unlink()
    elif target.exists():
        shutil.rmtree(target)

    target.parent.mkdir(exist_ok=True, parents=True)
    target.symlink_to(overlay_root)
    print(f"Symlinked: {target} -> {overlay_root}")


if __name__ == "__main__":
    main()
