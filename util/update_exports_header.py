#!/usr/bin/env python3
"""Regenerate include/workspace/exports.h from EXPORTC lines in qlogo/workspace/exports.cpp."""

import subprocess
from pathlib import Path

HEADER = """#ifndef WORKSPACE_EXPORTS_H
#define WORKSPACE_EXPORTS_H

#include "compiler_types.h"
#include "workspace/visited.h"

#include <QString>

#ifdef _WIN32
#define EXPORTC extern "C" __declspec(dllexport)
#else
#define EXPORTC extern "C"
#endif

"""

FOOTER = """#endif // WORKSPACE_EXPORTS_H
"""

def _repo_root() -> Path:
    result = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        capture_output=True,
        text=True,
        check=True,
        cwd=Path(__file__).resolve().parent,
    )
    return Path(result.stdout.strip())

REPO_ROOT = _repo_root()
HEADER_PATH = REPO_ROOT / "include" / "workspace" / "exports.h"
SOURCE_PATH = REPO_ROOT / "qlogo" / "workspace" / "exports.cpp"


def extract_export_declarations(source_lines: list[str]) -> list[str]:
    """Collect EXPORTC lines from source, normalize to declarations (semicolon, no brace/comments)."""
    declarations = []
    for line in source_lines:
        stripped = line.lstrip()
        if not stripped.startswith("EXPORTC"):
            continue
        sig = stripped.rstrip("\n\r")
        if "//" in sig:
            sig = sig.split("//", 1)[0].rstrip()
        if sig.endswith("{"):
            sig = sig[:-1].rstrip()
        if not sig.endswith(";"):
            sig += ";"
        declarations.append(sig + "\n")
    return declarations


def main() -> None:
    source_lines = SOURCE_PATH.read_text(encoding="utf-8").splitlines(keepends=True)
    body = "".join(extract_export_declarations(source_lines))
    HEADER_PATH.write_text(HEADER + body + FOOTER, encoding="utf-8")


if __name__ == "__main__":
    main()
