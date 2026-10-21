#!/usr/bin/env python3
"""Split seeds.txt on '===' into one file per seed under a corpus directory."""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

DELIMITER = b"==="


def split_seeds(data: bytes) -> list[bytes]:
    seeds: list[bytes] = []
    for chunk in data.split(DELIMITER):
        chunk = chunk.lstrip(b"\r\n").rstrip(b"\r\n")
        if chunk:
            seeds.append(chunk)
    return seeds


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("seeds_file", type=Path)
    parser.add_argument("corpus_dir", type=Path)
    parser.add_argument("stamp_file", type=Path)
    args = parser.parse_args()

    if not args.seeds_file.is_file():
        print(f"Seeds file not found: {args.seeds_file}", file=sys.stderr)
        return 1

    seeds = split_seeds(args.seeds_file.read_bytes())
    if not seeds:
        print(f"No seeds found in {args.seeds_file}", file=sys.stderr)
        return 1

    if args.corpus_dir.exists():
        shutil.rmtree(args.corpus_dir)
    args.corpus_dir.mkdir(parents=True)

    for index, seed in enumerate(seeds, start=1):
        (args.corpus_dir / f"seed_{index}").write_bytes(seed)

    args.stamp_file.write_text(f"{len(seeds)}\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
